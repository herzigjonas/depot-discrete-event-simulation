# Logistics Depot Simulation (SHO Model)

A discrete-event simulation of a parcel delivery depot, modeling truck arrivals, package sorting, and regional dispatch — built to identify bottlenecks and evaluate capacity-planning scenarios.

Developed as a project for the *Modeling and Simulation* course at Brno University of Technology, Faculty of Information Technology.

**Team project** — co-authored with [Lukáš Procházka](https://github.com/LukaskovoGitHub) (xproch0u).

## What it models

The simulation follows a parcel's full journey through a logistics depot:

1. **Truck arrival & unloading** — trucks arrive at random (exponential) intervals, carrying 800–1400 packages each, unloaded onto a limited number of ramps
2. **Sorting** — packages are classified as express (priority) or standard, then routed to one of 4 regional destinations
3. **Regional dispatch** — each region batches packages into vehicles, which depart once full (80 packages) or after a 4-hour timeout, whichever comes first
4. **Delivery** — vehicles are unavailable for 5–8 hours per trip before returning to the depot

The model tracks waiting times, resource utilization, and throughput to identify system bottlenecks.

## Tech stack

- **C++11**
- **[SIMLIB](http://simlib.fit.vutbr.cz/)** — a discrete-event simulation library developed at FIT VUT, providing `Process`, `Event`, `Store`, and `Facility` primitives for modeling queues, resources, and timed behavior

## Build & run

```
make
make run
```

Requires SIMLIB installed and linkable via `-lsimlib`.

## Key findings

Three scenarios were simulated and compared (see [`dokumentace.pdf`](./dokumentace.pdf) for full methodology and results):

| Scenario | Avg. package time at depot | Avg. vehicles in the field |
|---|---|---|
| Current state (8 vehicles) | 216 min | 5.57 |
| Increased fleet (20 vehicles) | 45.3 min | 12.68 |
| 2x package volume (current fleet) | 137 min | 17.34 |

**Conclusion:** the depot's ramps and sorting handle normal and even doubled load without issue, but **vehicle capacity is the primary bottleneck** — increasing the fleet size dramatically reduced package dwell time at the depot, while a 2x volume increase without added vehicles pushed the fleet back toward its limits.

## Documentation

Full write-up (system description, Petri net diagrams, implementation details, and experiment results) is available in Czech in [`dokumentace.pdf`](./dokumentace.pdf).
