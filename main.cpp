// Jonáš Herzig, Lukáš Procházka
// xherzi00, xproch0u
#include "simlib.h"
#include <iostream>
#include <vector>

using namespace std;

const double DOBA_TRIDENI = 2.0/60.0;
const double DOBA_NAKLADKY = 15.0;
const double DOBA_CEKANI = 240.0;
const int MIN_BALIKU = 800;
const int MAX_BALIKU = 1400;

Store Rampa("Rampa", 8);
Store Auta("Auta", 20);
Facility Tridicka("Tridicka");

Stat StatPocetBalikuVKamionu("Baliku v kamionu [ks]");
Stat StatExpresnich("Expresni baliky [ks]");
Stat StatNormalnich("Normalni baliky [ks]");
Stat StatRegion1("Baliky R1 [ks]");
Stat StatRegion2("Baliky R2 [ks]");
Stat StatRegion3("Baliky R3 [ks]");
Stat StatRegion4("Baliky R4 [ks]");
Histogram HistVelikostKamionu("Velikost kamionu", MIN_BALIKU, 100, (MAX_BALIKU-MIN_BALIKU)/10);
Stat StatCasPrijezduKamionu("Cas prijezdu kamionu [min]");

// Statistika rozvozu a jízd aut
Stat StatRozvezenoCelkem("Rozvezeno celkem [ks]");    
Stat StatRozvezenoR1("Rozvezeno R1 [ks]");
Stat StatRozvezenoR2("Rozvezeno R2 [ks]");
Stat StatRozvezenoR3("Rozvezeno R3 [ks]");
Stat StatRozvezenoR4("Rozvezeno R4 [ks]");

Stat StatPocetJizd("Pocet jizd [#]");          
Stat StatPocetJizdR1("Pocet jizd R1 [#]");
Stat StatPocetJizdR2("Pocet jizd R2 [#]");
Stat StatPocetJizdR3("Pocet jizd R3 [#]");
Stat StatPocetJizdR4("Pocet jizd R4 [#]");

Stat StatTrvaniJizdy("Trvani jizdy [min]");        
Stat StatTrvaniJizdyR1("Trvani jizdy R1 [min]");
Stat StatTrvaniJizdyR2("Trvani jizdy R2 [min]");
Stat StatTrvaniJizdyR3("Trvani jizdy R3 [min]");
Stat StatTrvaniJizdyR4("Trvani jizdy R4 [min]");
Stat StatCekaniBaliku("Doba cekani baliku [min]");

class JizdaAuta : public Process {
    Store& auta;
    int region; 
public:
    JizdaAuta(Store& s, int r) : auta(s), region(r) {}
    void Behavior() override {
        double t = Uniform(5*60.0, 8*60.0);
        StatPocetJizd(1);
        StatTrvaniJizdy(t);
        switch(region){
            case 1: StatPocetJizdR1(1); StatTrvaniJizdyR1(t); break;
            case 2: StatPocetJizdR2(1); StatTrvaniJizdyR2(t); break;
            case 3: StatPocetJizdR3(1); StatTrvaniJizdyR3(t); break;
            case 4: StatPocetJizdR4(1); StatTrvaniJizdyR4(t); break;
        }
        Wait(t);
        Leave(auta, 1);  
    }
};

class DispecerRegionu : public Process {
    Store& auta;
    const char* jmeno;
    int region; 
public:
    int pocet = 0;
    // FIFO časů příchodu balíků do regionu
    std::vector<double> arrivalTimes;

    // stav várky
    double deadline = -1.0;
    int batch_id = 0;         // identifikátor aktuální várky
    bool timeoutFlag = false; // budík vystřelil pro aktuální várku

    // budík pro timeout, aby se neaktivoval přímo tento proces dvakrát
    struct Budik : public Event {
        DispecerRegionu* d = nullptr;
        int id = 0;
        explicit Budik(DispecerRegionu* dd = nullptr) : d(dd) {}
        void Nastav(DispecerRegionu* dd, int newId) { d = dd; id = newId; }
        void Behavior() override { d->OnTimeout(id); }
    } budik;

    DispecerRegionu(Store& a, const char* nm, int r) : auta(a), jmeno(nm), region(r), budik(this) {}

    // volá budík po 4 h
    void OnTimeout(int id) {
        if (id == batch_id) {          // ignoruj staré budíky
            timeoutFlag = true;
            if (Idle() && Where() == 0)
                Activate();
        }
    }

    // zavolat při příchodu balíku do regionu
    void PridejBalik() {
        if (pocet == 0) {
            deadline = Time + DOBA_CEKANI;
            ++batch_id;                        // nová várka => nový identifikátor
            budik.Nastav(this, batch_id);
            budik.Activate(deadline);         
        }
        pocet++;
        arrivalTimes.push_back(Time);          // ulož čas příchodu balíku
        if (Idle() && Where() == 0)
            Activate();
    }

    void Behavior() override {
        while (true) {
            Passivate();

            while ((pocet >= 80) || (timeoutFlag && pocet > 0)) {
                Enter(auta, 1);                 
                int nalozit = pocet >= 80 ? 80 : pocet;
                Wait(DOBA_NAKLADKY);            
                StatRozvezenoCelkem(nalozit);
                switch(region){
                    case 1: StatRozvezenoR1(nalozit); break;
                    case 2: StatRozvezenoR2(nalozit); break;
                    case 3: StatRozvezenoR3(nalozit); break;
                    case 4: StatRozvezenoR4(nalozit); break;
                }
                // zapiš dobu čekání pro naložené balíky
                for (int i = 0; i < nalozit && !arrivalTimes.empty(); ++i) {
                    double t_in = arrivalTimes.front();
                    arrivalTimes.erase(arrivalTimes.begin());
                    double waitTime = Time - t_in;
                    StatCekaniBaliku(waitTime);
                }
                (new JizdaAuta(auta, region))->Activate(); // auto odjede a po 5–8 h vrátí kapacitu

                pocet -= nalozit;

                // restart časovače pro případné zbylé kusy
                if (pocet > 0) {
                    deadline = Time + DOBA_CEKANI;
                    timeoutFlag = false;
                    ++batch_id;
                    budik.Nastav(this, batch_id);
                    budik.Activate(deadline);
                } else {
                    deadline = -1.0;
                    timeoutFlag = false;
                }
            }
        }
    }
    //započítá čekání i pro balíky, které neodjely
    void FlushWaitingStatsOnEnd(double simEndTime) {
        while (!arrivalTimes.empty()) {
            double t_in = arrivalTimes.front();
            arrivalTimes.erase(arrivalTimes.begin());
            double waitTime = simEndTime - t_in;
            StatCekaniBaliku(waitTime);
        }
    }
};

DispecerRegionu* R1;
DispecerRegionu* R2;
DispecerRegionu* R3;
DispecerRegionu* R4;

struct KonecSimulaceFlush : public Event {
    void Behavior() override {
        double simEnd = Time;
        if (R1) R1->FlushWaitingStatsOnEnd(simEnd);
        if (R2) R2->FlushWaitingStatsOnEnd(simEnd);
        if (R3) R3->FlushWaitingStatsOnEnd(simEnd);
        if (R4) R4->FlushWaitingStatsOnEnd(simEnd);
    }
} EndFlush;

class Balik : public Process {
    bool expres;
public:
    Balik(bool e) : expres(e) {}
    void Behavior() override {
        this->Priority = expres ? 1 : 0;
        Seize(Tridicka);
        Wait(DOBA_TRIDENI);
        Release(Tridicka);
        double r = Random();
        if (r < 0.30) {
            StatRegion1(1);
            R1->PridejBalik();
        } else if (r < 0.55) {
            StatRegion2(1);
            R2->PridejBalik();
        } else if (r < 0.75) {
            StatRegion3(1);
            R3->PridejBalik();
        } else {
            StatRegion4(1);
            R4->PridejBalik();
        }

    }
};

class VykladkaKamionu : public Process {
    int pocet;
public:
    VykladkaKamionu(int p) : pocet(p) {}
    void Behavior() override {
        Wait(Uniform(30.0, 90.0));
        for (int i = 0; i < pocet; ++i) {
            bool jeExpres = (Random() < 0.20); // 20 % expresních
            if (jeExpres) 
            {
                StatExpresnich(1);
            }
            else          
            {
                StatNormalnich(1);
            }
            (new Balik(jeExpres))->Activate();
        }
        Leave(Rampa, 1);
    }
};

class PrijezdKamionu : public Process {
    void Behavior() override {
        Wait(Exponential(540));
        StatCasPrijezduKamionu(Time); 
        Enter(Rampa, 1);
        int pocetBaliku = (int)Uniform(MIN_BALIKU, MAX_BALIKU + 1);
        StatPocetBalikuVKamionu(pocetBaliku);
        HistVelikostKamionu(pocetBaliku);
        (new VykladkaKamionu(pocetBaliku))->Activate();
        (new PrijezdKamionu)->Activate();
    }
};

int main()
{
    RandomSeed(time(NULL));
    R1 = new DispecerRegionu(Auta, "Region 1", 1);
    R2 = new DispecerRegionu(Auta, "Region 2", 2);
    R3 = new DispecerRegionu(Auta, "Region 3", 3);
    R4 = new DispecerRegionu(Auta, "Region 4", 4);
    Init(0, 18*60);
    EndFlush.Activate(18*60);
    (new PrijezdKamionu)->Activate();
    Run();
    StatPocetBalikuVKamionu.Output();
    StatExpresnich.Output();
    StatNormalnich.Output();
    StatRegion1.Output();
    StatRegion2.Output();
    StatRegion3.Output();
    StatRegion4.Output();

    Rampa.Output();
    Auta.Output();
    Tridicka.Output();

    // Výstupy statistik rozvozu a jízd
    StatRozvezenoCelkem.Output();
    StatRozvezenoR1.Output();
    StatRozvezenoR2.Output();
    StatRozvezenoR3.Output();
    StatRozvezenoR4.Output();

    StatPocetJizd.Output();

    StatTrvaniJizdy.Output();
    StatCasPrijezduKamionu.Output();
    StatCekaniBaliku.Output();

}

