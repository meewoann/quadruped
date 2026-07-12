---
name: new-component
description: Scaffold a new ECS component header under include/gz/sim/components/, wire its CMake listing, and (if needed) register it with ComponentFactory for serialization. Trigger when the user asks to add a new component or a per-entity data field.
---

# Adding a new component

Components in gz-sim are header-only wrappers around the `Component<>`
template. They live in `include/gz/sim/components/<Name>.hh`.

## Decide first

1. **What's the data type?** A plain struct, a `math::Vector3d`, an
   `std::string`, a `msgs::*`?  Components don't have to be POD but they
   must be copyable.
2. **Does it need serialization?** If users should be able to log/replay
   the component or read it from SDF, you need to give it a
   `Serializer<T>` and register it with `ComponentFactory`.
3. **Is there a Cmd / Reset variant?** If this component represents
   physical state that physics writes, you probably also want
   `<Name>Cmd.hh` (user → physics) and `<Name>Reset.hh` (snapshot for
   `Reset()`).

## Template (no custom serializer)

```cpp
// include/gz/sim/components/Foo.hh
#ifndef GZ_SIM_COMPONENTS_FOO_HH_
#define GZ_SIM_COMPONENTS_FOO_HH_

#include <gz/sim/components/Component.hh>
#include <gz/sim/components/Factory.hh>
#include <gz/sim/config.hh>

namespace gz::sim
{
inline namespace GZ_SIM_VERSION_NAMESPACE {
namespace components
{
  /// \brief One-line description of what this component represents and
  ///        which entities it gets attached to.
  using Foo = Component<double, class FooTag>;
  GZ_SIM_REGISTER_COMPONENT("gz_sim_components.Foo", Foo)
}
}
}

#endif
```

Key points:

* The unique tag `class FooTag` is what prevents two components with the
  same data type from collapsing into one — never skip it.
* `GZ_SIM_REGISTER_COMPONENT(name, type)` is required for *any* component
  that needs to round-trip through `ComponentFactory`.

## Template (custom data type + serializer)

```cpp
#ifndef GZ_SIM_COMPONENTS_BAR_HH_
#define GZ_SIM_COMPONENTS_BAR_HH_

#include <gz/msgs/double.pb.h>
#include <gz/sim/components/Component.hh>
#include <gz/sim/components/Factory.hh>
#include <gz/sim/components/Serialization.hh>
#include <gz/sim/config.hh>

namespace gz::sim
{
inline namespace GZ_SIM_VERSION_NAMESPACE {
namespace components
{
  using BarSerializer = serializers::MsgsDoubleSerializer;
  using Bar = Component<msgs::Double, class BarTag, BarSerializer>;
  GZ_SIM_REGISTER_COMPONENT("gz_sim_components.Bar", Bar)
}
}
}

#endif
```

For brand-new data types, define `Serializer<T>` with
`static std::ostream &Serialize(...)` and
`static std::istream &Deserialize(...)` in
`include/gz/sim/components/Serialization.hh` style and reference it from
the component.

## CMake plumbing

Add the new header to
`include/gz/sim/components/CMakeLists.txt` so it gets installed:

```cmake
set (component_headers
  Actor.hh
  ...
  Foo.hh        # <-- new
  ...
)
```

Headers are picked up automatically by the install logic — just keep the
list alphabetical to match existing style.

## Tests

* Add a small `Foo_TEST.cc` next to the existing component tests under
  `src/components/` if there's non-trivial behaviour (e.g. operator==
  or a custom serializer). Trivial typedef components don't need a
  dedicated test.
* If the component is logged/replayed, add a round-trip test under
  `test/integration/log_system.cc` patterns.

## Migration note

If the component is part of the public API, append a one-line entry to
`Migration.md` under the next-release section so downstream users notice.

## Common mistakes

* Reusing an existing tag class — two components with the same tag are
  the *same* component.
* Forgetting the `GZ_SIM_REGISTER_COMPONENT` macro for a serialized type.
* Adding mutation logic into the component header — components are *data*,
  behaviour belongs in a system.
