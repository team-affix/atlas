# Bootstrap and Wiring

## `resolver`

All concrete objects are connected through a **`resolver`** — a global registry mapping `std::type_index` keys to object instances. Every constructor calls `resolver::resolve<T>()` to look up its dependencies rather than receiving them as parameters.

This means the code that constructs an object does not need to know what that object depends on — dependencies are pulled from the resolver inside the constructor itself. Construction order still matters; dependencies must be registered before their dependents are constructed.

## Manifests *(planned)*

The intended mechanism for populating the resolver is a **manifest** — a declarative description of which concrete type to bind to each interface, and in what order to construct objects. A manifest runs once at startup, before the solver begins, and registers all instances into the resolver.

Different manifests would wire different configurations — e.g. `horizon` vs `ridge`, or debug vs release logging.
