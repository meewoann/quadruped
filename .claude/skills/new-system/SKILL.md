---
name: new-system
description: Scaffold a new gz-sim system plugin under src/systems/<name>/ — header, source, CMake glue, plugin registration, and an integration test stub. Trigger when the user asks to create a new system, controller, or plugin that hooks into the simulation loop.
---

# Adding a new system plugin

A system in gz-sim is a shared library that implements one or more of
`ISystemConfigure`, `ISystemPreUpdate`, `ISystemUpdate`, `ISystemPostUpdate`,
`ISystemReset` and gets loaded via SDF `<plugin>` or `<server_config>`.

Directory convention: `src/systems/<snake_case>/<CamelCase>.{hh,cc}`.

## Step 1 — pick the directory + filename

* Directory: `snake_case` matching the plugin's purpose (e.g.
  `fancy_drive`, `wheel_slip`).
* Class: `CamelCase` matching the directory (e.g. `FancyDrive`).
* SDF plugin name: `gz::sim::systems::<CamelCase>`.

## Step 2 — header template (`FancyDrive.hh`)

```cpp
#ifndef GZ_SIM_SYSTEMS_FANCYDRIVE_HH_
#define GZ_SIM_SYSTEMS_FANCYDRIVE_HH_

#include <memory>

#include <gz/sim/System.hh>
#include <gz/sim/config.hh>
#include <gz/utils/ImplPtr.hh>

namespace gz::sim
{
inline namespace GZ_SIM_VERSION_NAMESPACE {
namespace systems
{
  class FancyDrivePrivate;

  /// \brief One-paragraph description of what this system does.
  ///
  /// SDF parameters:
  ///   <param_a>  — what it does (default: ...)
  ///   <param_b>  — what it does (default: ...)
  class FancyDrive
    : public System,
      public ISystemConfigure,
      public ISystemPreUpdate,
      public ISystemPostUpdate
  {
    public: FancyDrive();
    public: ~FancyDrive() override;

    // System hooks
    public: void Configure(const Entity &_entity,
                           const std::shared_ptr<const sdf::Element> &_sdf,
                           EntityComponentManager &_ecm,
                           EventManager &_eventMgr) override;

    public: void PreUpdate(const UpdateInfo &_info,
                           EntityComponentManager &_ecm) override;

    public: void PostUpdate(const UpdateInfo &_info,
                            const EntityComponentManager &_ecm) override;

    /// \brief Private data via PIMPL.
    GZ_UTILS_UNIQUE_IMPL_PTR(dataPtr)
  };
}
}
}

#endif
```

## Step 3 — source template (`FancyDrive.cc`)

```cpp
#include "FancyDrive.hh"

#include <gz/plugin/Register.hh>
#include <gz/sim/Model.hh>
#include <gz/sim/components/JointVelocityCmd.hh>

namespace gz::sim::systems
{
class FancyDrivePrivate
{
  public: Model model{kNullEntity};
  // …cached handles, sdf params, transport node, etc.
};

FancyDrive::FancyDrive() : dataPtr(gz::utils::MakeUniqueImpl<FancyDrivePrivate>())
{
}

FancyDrive::~FancyDrive() = default;

void FancyDrive::Configure(const Entity &_entity,
                           const std::shared_ptr<const sdf::Element> &_sdf,
                           EntityComponentManager &_ecm,
                           EventManager &/*_eventMgr*/)
{
  this->dataPtr->model = Model(_entity);
  if (!this->dataPtr->model.Valid(_ecm))
  {
    gzerr << "FancyDrive should be attached to a model entity. "
          << "Failed to initialize." << std::endl;
    return;
  }
  // parse _sdf params here…
}

void FancyDrive::PreUpdate(const UpdateInfo &_info,
                           EntityComponentManager &_ecm)
{
  if (_info.paused) return;
  // mutate _ecm: set joint velocity cmds, etc.
}

void FancyDrive::PostUpdate(const UpdateInfo &_info,
                            const EntityComponentManager &_ecm)
{
  // read-only — publish telemetry, log, etc.
}
}  // namespace gz::sim::systems

GZ_ADD_PLUGIN(gz::sim::systems::FancyDrive,
              gz::sim::System,
              gz::sim::systems::FancyDrive::ISystemConfigure,
              gz::sim::systems::FancyDrive::ISystemPreUpdate,
              gz::sim::systems::FancyDrive::ISystemPostUpdate)

GZ_ADD_PLUGIN_ALIAS(gz::sim::systems::FancyDrive,
                    "gz::sim::systems::FancyDrive")
```

The alias is what users actually type in their `<plugin filename="..."
name="gz::sim::systems::FancyDrive">` tags — keep it stable across releases.

## Step 4 — local CMake (`src/systems/fancy_drive/CMakeLists.txt`)

```cmake
gz_add_system(fancy-drive
  SOURCES
    FancyDrive.cc
  PUBLIC_LINK_LIBS
    gz-common${GZ_COMMON_VER}::gz-common${GZ_COMMON_VER}
    gz-math${GZ_MATH_VER}::gz-math${GZ_MATH_VER}
    gz-transport${GZ_TRANSPORT_VER}::gz-transport${GZ_TRANSPORT_VER}
)
```

Use existing sibling directories as the source of truth — the macro name
and link-libs list differs by feature set.

## Step 5 — register with parent CMake (`src/systems/CMakeLists.txt`)

Add an `add_subdirectory(fancy_drive)` entry alphabetically.

## Step 6 — integration test (`test/integration/fancy_drive_system.cc`)

Minimum viable test:

```cpp
#include <gtest/gtest.h>
#include <gz/sim/Server.hh>
#include <gz/sim/TestFixture.hh>

TEST(FancyDriveTest, LoadsCleanly)
{
  gz::sim::TestFixture fixture("test/worlds/fancy_drive.sdf");
  fixture.Server()->Run(true, 100, false);
  // assertions on telemetry / final pose
}
```

Add the target in `test/integration/CMakeLists.txt`.

## Step 7 — example world

Add `examples/worlds/fancy_drive.sdf` with the minimum SDF a user needs
to try the system, and reference it from a short tutorial under
`tutorials/`.

## Step 8 — `Migration.md`

If the system is publicly named or replaces an older one, append a note
to `Migration.md`.

## Don't forget

* `pre-commit run --files <changed files>` before declaring done.
* Add the new system to `Changelog.md` under the upcoming-release section.
