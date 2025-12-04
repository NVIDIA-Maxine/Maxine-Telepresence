# Entity Component System

The operation of the nv3dvc::core::engine::Engine is the execution of Systems acting on Components. This is the basis of the Entity-Component-System (ECS) framework.
An entity is defined by a unique ID to a registry where all components attached to a given entity can be fetched and acted upon.
The design pattern favors composition over inheritance, and components can be added or removed programmatically.
A scene structure can define any set of entities with components attached.
Typically, data and properties are stored within components, and algorithms are defined in the systems' Run function.

See:
- nv3dvc::core::ecs::Entity
- nv3dvc::core::ecs::Component
- nv3dvc::core::ecs::System

A special type of component is the behavior component. See
- nv3dvc::modules::behaviormodule::components::BehaviorComponent

Components, behaviors, and system classes can be extended for special use cases. These are grouped within modules. See nv3dvc::core::engine::Module
