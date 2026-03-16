# Unreal Engine C++ Vehicle Physics Simulation
I wanted to assemble my own physics vehicle system and not use the already existing one (`VehicleMovementComponent`) in order to have a **custom and finely tuned vehicle simulation** suited to my needs.
The vehicle architecture is **component based**, with a custom `PawnMovementComponent` that manages all vehicle components.

---

## Architecture
The vehicle is implemented using a **modular component based structure**.
```text
Vehicle Pawn
└── Custom PawnMovementComponent
    ├── Wheel System (suspension, friction, steering, surface detection)
    ├── Engine System (gearbox, power transmission to the wheels, engine simulation)
    ├── Aerodynamic System (drag force and downforce at high speed)
    └── Input System (throttle, steering, brake, handbrake, engine toggle)
```

Each component is responsible for a specific aspect of the vehicle simulation while remaining **independent and configurable**.

---

## More Technical Details
For those interested in the technical side, I go into more detail in my portfolio:
- Portfolio page:  
  https://sites.google.com/view/tupinon-alois-portfolio/home/no-ledge/custom-physics-based-tire-vehicle-system
- Short demo video:  
  https://www.youtube.com/watch?v=4DYZ8WPVLeU
