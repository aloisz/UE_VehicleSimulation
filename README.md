Unreal Engine C++ Vehicle Physics Simulation

I wanted to assemble my own physics vehicle system and not use the already existing one (vehicle movement component) to have custom and fined tune vehicle for my needs. 
The vehicle architecture is component based with a PawnMovementcomponent that hold all the components. 

== Architecture == 
The vehicle is implemented using a modular component based structure.

Vehicle Pawn
 └── Custom PawnMovementComponent
      ├── Wheel System (suspension, frictions, steering, surface detection)
      ├── Engine System (Gearbox, power transmission to the wheels, engine simulation)
      ├── Aerodynamic System (simulate forces that affect vehicle at high speed, drag force and downforce)
      └── Input System (Throttle, steering, brake, handbrake, engine toggle)

Each subsystem is responsible for a specific aspect of the vehicle simulation while remaining independent and configurable.


For those interested in the technical side, I go into more detail in my portfolio :
- https://sites.google.com/view/tupinon-alois-portfolio/home/no-ledge/custom-physics-based-tire-vehicle-system
- Short demo video : https://www.youtube.com/watch?v=4DYZ8WPVLeU 
