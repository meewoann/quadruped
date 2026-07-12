# VDA 5050 v3.0.0 — Complete Message & Process Reference

Exhaustive field-by-field reference for **all eight** VDA 5050 messages
and **every communication process**. Companion to the conceptual
overview in `vda5050_protocol.md`. Ground truth:
`~/nav2_ws/src/VDA5050/VDA5050_EN.md` (§6 processes, §7 message spec) and
`~/nav2_ws/src/VDA5050/json_schemas/*.schema`.

**Notation:** `*field*` = optional · `**field**` = array · objects are
shown as `{ … }`. Every message also carries the §3 header
(`headerId`, `timestamp`, `version`, `manufacturer`, `serialNumber`) —
omitted below except where noted.

---

## A. `order` (fleet → robot) · `order.schema`

```
order { header…, orderId, orderUpdateId, *orderDescription*,
        nodes[node], edges[edge] }
```

### node
| Field | Unit | Type | Notes |
|-------|------|------|-------|
| `nodeId` | | string | May repeat; disambiguated by `sequenceId`. |
| `sequenceId` | | uint32 | Shared node/edge traversal order. |
| `*nodeDescriptor*` | | string | Human-readable only. |
| `released` | | bool | `true`=base, `false`=horizon. |
| `*nodePosition*` | | object | Optional for line-guided robots. |
| `actions` | | array<action> | Empty if none. |

### nodePosition
| Field | Unit | Type | Notes |
|-------|------|------|-------|
| `x`,`y` | m | float64 | Project-specific global coordinates. |
| `*theta*` | rad | float64 | `[-π,π]`; orientation to match on the node. |
| `*allowedDeviationXY*` | | object | Ellipse (see below). |
| `*allowedDeviationTheta*` | rad | float64 | `[0,π]`; `0.0` = exact. |
| `mapId` | | string | Map the position references. |

### allowedDeviationXY (ellipse, §6.6.2.1)
`{ a (m), b (m), theta (rad) }` — semi-major `a`, semi-minor `b`,
rotation `theta` from the +x axis. `a=b=0.0` ⇒ match as precisely as
technically possible. Node center = ellipse center.

### edge
| Field | Unit | Type | Notes |
|-------|------|------|-------|
| `edgeId` | | string | May repeat; `sequenceId` disambiguates. |
| `sequenceId` | | uint32 | Shared with nodes. |
| `*edgeDescriptor*` | | string | Human-readable. |
| `released` | | bool | base / horizon. |
| `startNodeId` / `endNodeId` | | string | Connected nodes (directional). |
| `*maximumSpeed*` | m/s | float64 | Speed cap on the edge. |
| `*maximumMobileRobotHeight*` | m | float64 | Incl. load. |
| `*minimumLoadHandlingDeviceHeight*` | m | float64 | |
| `*orientation*` | rad | float64 | Interpreted via `orientationType`. |
| `*orientationType*` | | enum | `GLOBAL`(omni)·`TANGENTIAL`(default; 0=fwd, π=back). |
| `*direction*` | | string | Junction dir for line-guided (`"left"`,`"580 Hz"`). |
| `*reachOrientationBeforeEntering*` | | bool | Omni only; default `false`. |
| `*maximumRotationSpeed*` | rad/s | float64 | |
| `*trajectory*` | | object | NURBS (see below). |
| `*length*` | m | float64 | Line-guided slow-down hint. |
| `*corridor*` | | object | Deviation bounds (see below). |
| `actions` | | array<action> | Active only while traversing the edge. |

### trajectory (NURBS) — used in order edges, `edgeState`, `plannedPath`
| Field | Type | Notes |
|-------|------|-------|
| `*degree*` | uint32 | `[1 … ]`, default 1. |
| `knotVector` | array<float64> | Clamped, `size = #controlPoints + degree + 1`, range `[0,1]`. |
| `controlPoints` | array<controlPoint> | ≥ `degree + 1`. |

**controlPoint:** `{ x (m), y (m), *weight* }` — weight range `]0 … ]`,
default `1.0`.

### corridor (§6.1.5)
| Field | Unit | Type | Notes |
|-------|------|------|-------|
| `leftWidth` / `rightWidth` | m | float64 | `[0 … ]`, deviation bounds L/R of the trajectory. |
| `*corridorReferencePoint*` | | enum | `KINEMATIC_CENTER`(default)·`CONTOUR`. |

A corridor in an order is released by default; if `releaseRequired` is
`true` the robot must request approval via `edgeRequest` before
deviating (§6.6.10). Leaving the corridor ⇒ stop + error
`OUTSIDE_OF_CORRIDOR` (`CRITICAL`).

### action (also used in instantActions, zoneActions)
| Field | Type | Notes |
|-------|------|-------|
| `actionType` | string | Function id (predefined or custom). |
| `actionId` | string | Unique; maps to `actionState`. Use UUIDs. |
| `*actionDescriptor*` | string | Human-readable. |
| `blockingType` | enum | `NONE`·`SINGLE`·`SOFT`·`HARD`. |
| `*actionParameters*` | array | `[{key, value}]`; `value` ∈ array/bool/number/integer/string/object. |
| `*retriable*` | bool | `true` → can enter `RETRIABLE`. Default `false`. |

`actionParameters` example:
`[{"key":"stationType","value":"floor"},{"key":"weight","value":8.5}]`.

---

## B. `instantActions` (fleet → robot) · `instantActions.schema`

```
instantActions { header…, actions[action] }
```

Same `action` object as orders, executed immediately regardless of graph
progress. Reported in `state.instantActionStates[]` until
`clearInstantActions`.

---

## C. `state` (robot → fleet) · `state.schema`

Top-level fields (header omitted):

| Field | Type | Notes |
|-------|------|-------|
| `*maps*` | array<map> | Maps stored on the robot. |
| `*zoneSets*` | array<zoneSet> | Zone sets stored on the robot. |
| `orderId` / `orderUpdateId` | string / uint32 | Current/last; `""`/`0` if none. |
| `lastNodeId` / `lastNodeSequenceId` | string / uint32 | Last reached/current node. |
| `nodeStates` | array<nodeState> | Remaining nodes (empty = idle). |
| `edgeStates` | array<edgeState> | Remaining edges (empty = idle). |
| `*plannedPath*` | object | NURBS + `traversedNodes[nodeId]`. |
| `*intermediatePath*` | object | `polyline[waypoint]` with ETAs. |
| `*mobileRobotPosition*` | object | Omit only for non-localizing robots. |
| `*velocity*` | object | `{ *vx*, *vy*, *omega* }` robot-frame. |
| `*loads*` | array<load> | Omit if unknown; `[]` = confirmed unloaded. |
| `driving` | bool | Driving (manual or automatic). |
| `*paused*` | bool | Paused (button or instant action). |
| `*newBaseRequest*` | bool | Near end of base — fleet should extend. |
| `*zoneRequests*` | array<zoneRequest> | Active interactive-zone requests. |
| `*edgeRequests*` | array<edgeRequest> | Active corridor requests. |
| `*distanceSinceLastNode*` | float64 (m) | Line-guided robots. |
| `actionStates` | array<actionState> | All order actions. |
| `instantActionStates` | array<actionState> | Until `clearInstantActions`. |
| `*zoneActionStates*` | array<actionState> | Required if action zones supported. |
| `powerSupply` | object | Battery/charge. |
| `operatingMode` | enum | See §F. |
| `errors` | array<error> | `[]` = no active errors. |
| `*information*` | array<info> | Debug/viz only — never for fleet logic. |
| `safetyState` | object | E-stop / field violation. |

### map
`{ mapId, mapVersion, mapStatus(ENABLED/DISABLED), *mapDescriptor* }`.
At most one map per `mapId` may be `ENABLED`.

### zoneSet (in state)
`{ zoneSetId, mapId, zoneSetStatus(ENABLED/DISABLED) }`. At most one zone
set per `mapId` may be `ENABLED`.

### nodeState
`{ nodeId, sequenceId, *nodeDescriptor*, released, *nodePosition* }`.

### edgeState
`{ edgeId, sequenceId, *edgeDescriptor*, released, *trajectory* }`.

### mobileRobotPosition
| Field | Unit | Type | Notes |
|-------|------|------|-------|
| `x`,`y` | m | float64 | Map coordinates. |
| `theta` | rad | float64 | `[-π,π]`. |
| `mapId` | | string | |
| `localized` | | bool | `false` ⇒ robot must report `LOCALIZATION_ERROR`(FATAL) and not drive. |
| `*localizationScore*` | | float64 | `[0,1]`, logging/viz only. |
| `*deviationRange*` | m | float64 | Logging/viz only. |

### load
| Field | Unit | Type | Notes |
|-------|------|------|-------|
| `*loadId*` | | string | Barcode/RFID; `""` if not yet identified. |
| `*loadType*` | | string | |
| `*loadPosition*` | | string | e.g. `"front"`, `"positionC1"`. |
| `*boundingBoxReference*` | | object | `{ x, y, z, *theta* }` (center of bottom face). |
| `*loadDimensions*` | | object | `{ length(m), width(m), *height(m)* }`. |
| `*weight*` | kg | float64 | `[0 … ]`. |

### actionState
`{ actionId, *actionType*, *actionDescriptor*, actionStatus, *actionResult* }`.
`actionStatus` ∈ `WAITING·INITIALIZING·RUNNING·PAUSED·RETRIABLE·FINISHED·FAILED`
(state machine in `vda5050_protocol.md` §5.1).

### powerSupply
| Field | Unit | Type | Notes |
|-------|------|------|-------|
| `stateOfCharge` | % | float64 | `[0,100]`; 100 for permanently powered. |
| `*batteryVoltage*` | V | float64 | |
| `*batteryCurrent*` | A | float64 | |
| `*batteryHealth*` | % | int8 | `[0,100]`. |
| `charging` | | bool | `false` only if available for orders. |
| `*range*` | m | uint32 | Estimated remaining range. |

### error
| Field | Type | Notes |
|-------|------|-------|
| `errorType` | string | Extensible enum (see §E). |
| `*errorReferences*` | array | `[{ referenceKey, referenceValue }]` (e.g. `nodeId`,`actionId`). |
| `*errorDescription*` | string | Cause detail. |
| `*errorDescriptionTranslations*` | array | `[{ translationKey(ISO 639-1), translationValue }]`. |
| `*errorHint*` | string | How to resolve. |
| `*errorHintTranslations*` | array | translations of the hint. |
| `errorLevel` | enum | `WARNING·URGENT·CRITICAL·FATAL` (see §E). |

### info
`{ infoType, *infoReferences*[{referenceKey,referenceValue}], *infoDescriptor*, infoLevel(DEBUG/INFO) }`.

### safetyState
`{ activeEmergencyStop(MANUAL/REMOTE/NONE), fieldViolation(bool) }`.

### intermediatePath / waypoint
`intermediatePath { polyline[waypoint] }`;
`waypoint { x(m), y(m), *theta*(rad), eta(ISO 8601 timestamp) }`.

---

## D. `visualization` (robot → viz) · `visualization.schema`

```
visualization { header…, referenceStateHeaderId,
                *plannedPath*, *intermediatePath*,
                *mobileRobotPosition*, *velocity* }
```

High-frequency position/path stream. Sub-objects reuse the `state`
definitions. No order logic depends on it.

---

## E. `connection` (broker/robot → fleet) · `connection.schema`

```
connection { header…, connectionState }
```

`connectionState` ∈ `ONLINE·OFFLINE·HIBERNATING·CONNECTION_BROKEN`.
Sent QoS 1; `CONNECTION_BROKEN` is the MQTT **last will**.

---

## F. `factsheet` (robot → fleet) · `factsheet.schema`

Capability advertisement, sent **retained** on connect / on
`factsheetRequest`.

```
factsheet { header…, typeSpecification, physicalParameters,
            protocolLimits, protocolFeatures, mobileRobotGeometry,
            loadSpecification, *mobileRobotConfiguration* }
```

### typeSpecification
`seriesName`, `*seriesDescription*`,
`mobileRobotKinematics`(enum `DIFFERENTIAL·OMNIDIRECTIONAL·THREE_WHEEL·…`),
`mobileRobotClass`(enum `FORKLIFT·CONVEYOR·TUGGER·CARRIER·…`),
`maximumLoadMass`(kg),
`localizationTypes`[enum `NATURAL·REFLECTOR·RFID·DMC·SPOT·GRID·…`],
`navigationTypes`[enum `PHYSICAL_LINE_GUIDED·VIRTUAL_LINE_GUIDED·FREELY_NAVIGATING·…`],
`*supportedZones*`[zone-type enum].

### physicalParameters
`minimumSpeed`, `maximumSpeed`, `*minimumAngularSpeed*`,
`*maximumAngularSpeed*`, `maximumAcceleration`, `maximumDeceleration`,
`minimumHeight`, `maximumHeight`, `width`, `length` (SI units).

### protocolLimits
- `maximumStringLengths { *maximumMessageLength*, *maximumTopicSerialLength*, *maximumTopicElementLength*, *maximumIdLength*, *idNumericalOnly*, *maximumLoadIdLength* }`
- `maximumArrayLengths { order.nodes, order.edges, node.actions, edge.actions, actions.actionsParameters, instantActions, trajectory.knotVector, trajectory.controlPoints, zoneSet.zones, state.nodeStates, state.edgeStates, state.loads, state.actionStates, state.instantActionStates, state.zoneActionStates, state.errors, state.information, error.errorReferences, information.infoReferences }` (all uint32; absent/0 = no limit)
- `timing { minimumOrderInterval, minimumStateInterval, *defaultStateInterval*, *visualizationInterval* }` (seconds)

### protocolFeatures
- `optionalParameters[{ parameter, support(SUPPORTED/REQUIRED), *description* }]`
  — params **not** listed are assumed unsupported.
- `mobileRobotActions[mobileRobotAction]`:
  `{ actionType, *actionDescription*, actionScopes[INSTANT/NODE/EDGE/ZONE],
     *actionParameters[{ key, valueDataType(BOOL/NUMBER/INTEGER/STRING/OBJECT/ARRAY), *description*, *isOptional* }]*,
     *actionResult*, *blockingTypes[NONE/SOFT/SINGLE/HARD]*,
     pauseAllowed, cancelAllowed }`.

### mobileRobotGeometry
- `*wheelDefinitions[{ type(DRIVE/CASTER/FIXED/MECANUM/…), isActiveDriven, isActiveSteered, position{x,y,*theta*}, diameter, width, *centerDisplacement*, *constraints* }]*`
- `*envelopes2d[{ envelope2dId, vertices[{x,y}], *description* }]*`
- `*envelopes3d[{ envelope3dId, format, *data*, *url*, *description* }]*`

### loadSpecification
- `*loadPositions*[string]` — valid values for `state.loads[].loadPosition` and `pick`/`drop` param `lhd`; empty ⇒ no load handling device.
- `*loadSets[{ setName, loadType, *loadPositions*, *boundingBoxReference*, *loadDimensions*, *maximumWeight*, *minimum/maximumLoadhandlingHeight*, *minimum/maximumLoadhandlingDepth*, *minimum/maximumLoadhandlingTilt*, *maximumSpeed*, *maximumAcceleration*, *maximumDeceleration*, *pickTime*, *dropTime*, *description* }]*`

### mobileRobotConfiguration (optional)
- `*versions[{ key, value }]*` — software/hardware versions.
- `*network { *dnsServers*, *ntpServers*, *localIpAddress*, *netmask*, *defaultGateway* }*`
- `*batteryCharging { *criticalLowChargingLevel*, *maximumDesiredChargingLevel*, *minimumDesiredChargingLevel*, *minimumChargingTime* }*`

---

## G. `zoneSet` (fleet → robot) · `zoneSet.schema`

```
zoneSet { header…, zoneSet{ mapId, zoneSetId, *zoneSetDescriptor*, zones[zone] } }
```

A `zoneSet` is immutable once given a `zoneSetId`; changes require a new
id. New zone sets arrive `DISABLED` and must be enabled via
`enableZoneSet`. Duplicate `zoneSetId` ⇒ error `DUPLICATE_ZONE_SET`
(`WARNING`).

### zone
| Field | Type | Notes |
|-------|------|-------|
| `zoneId` | string | Unique within the zone set. |
| `zoneType` | enum | See zone types below. |
| `*zoneDescriptor*` | string | Human-readable. |
| `vertices` | array<{x,y}> | Polygon, counter-clockwise, ≥3, simple (no self-intersection), closed. |
| `*maximumSpeed*` | float64 | `SPEED_LIMIT` only. |
| `*entryActions/duringActions/exitActions*` | array<zoneAction> | `ACTION` only. |
| `*releaseLossBehavior*` | enum | `RELEASE` only: `STOP·CONTINUE·EVACUATE`. |
| `*priorityFactor*` / `*penaltyFactor*` | float64 | `PRIORITY` / `PENALTY`, `[0,1]`. |
| `*direction*` | float64 | `DIRECTED`/`BIDIRECTED`, rad. |
| `*directedLimitation*` | enum | `DIRECTED`: `SOFT·RESTRICTED·STRICT`. |
| `*bidirectedLimitation*` | enum | `BIDIRECTED`: `SOFT·RESTRICTED`. |

`zoneAction` = `action` but the **robot** generates the `actionId`.

#### Zone types (§6.4.1)
**Contour-based** (robot contour incl. load decides entry/exit):
- `BLOCKED` — must not enter; violation ⇒ `BLOCKED_ZONE_VIOLATION`(CRITICAL).
- `LINE_GUIDED` — no free nav; follow fleet-provided node/edge graph + trajectory.
- `RELEASE` — enter only when granted; `releaseLossBehavior` on revoke/expire (`STOP`⇒`RELEASE_LOST` CRITICAL).
- `COORDINATED_REPLANNING` — no autonomous replanning without permission.
- `SPEED_LIMIT` — cap at `maximumSpeed` from entry.
- `ACTION` — run entry/during/exit actions.

**Kinematic-center-based** (center decides entry/exit):
- `PRIORITY` (`priorityFactor`) / `PENALTY` (`penaltyFactor`) — bias path planning.
- `DIRECTED` (`direction`, `directedLimitation`) — preferred travel direction.
- `BIDIRECTED` (`direction`, `bidirectedLimitation`) — direction and its opposite only.

---

## H. `responses` (fleet → robot) · `responses.schema`

```
responses { header…, responses[response] }
response { requestId, grantType, *leaseExpiry* }
```

`grantType` ∈ `GRANTED·QUEUED·REVOKED·REJECTED`. `leaseExpiry`
(ISO 8601) only with grants. Replies to robot-initiated requests carried
in `state.zoneRequests` / `state.edgeRequests`.

---

## I. Communication processes (§5–§6)

### Order lifecycle & base/horizon (§6.1.2)
- **Base** = released, committed route up to the **decision point**
  (last base node). The robot stops there if not extended. **Base is
  immutable** — fleet assumes it is already executed.
- **Horizon** = forecast (`released=false`); may change via order
  updates. Fleet should extend the base *before* the robot reaches the
  decision point (`newBaseRequest`/base-request trigger).
- **Order update / stitching:** same `orderId`, `orderUpdateId+1`; the
  first node of the update = the robot's current `lastNodeId` (decision
  node), then the new graph. To release actions on a node the robot
  already sits on, re-send the decision node, then add a node with the
  same position whose `sequenceId` = decision node `sequenceId + 2`.

### Order traversal (§6.6.2)
A node counts as traversed when the control point is within
`allowedDeviationXY` **and** orientation within `allowedDeviationTheta`.
On traversal the robot: removes the `nodeState`, sets
`lastNodeId`/`lastNodeSequenceId`, triggers node actions, removes the
incoming edge from `edgeStates` (finishing its actions), and enters the
next edge (triggering its actions) — unless a `SOFT`/`HARD` action stops
it on the node first. Markers not part of the active order's nodes do
**not** update `lastNodeId`.

### Order cancellation (§6.1.3)
`cancelOrder` instant action → robot stops ASAP / at next node →
becomes idle. Scheduled actions report `FAILED`; running actions are
cancelled (or finished if `cancelAllowed=false`); `cancelOrder` itself
reports `RUNNING` until all clear, then `FINISHED`. If idle / wrong
`orderId` ⇒ `cancelOrder` `FAILED` (+ `NO_ORDER_TO_CANCEL` WARNING).

### Order rejection (§6.1.4) → maps to error types in §E
Malformed (`VALIDATION_FAILURE`), unsupported action
(`INVALID_ORDER_ACTION`), unsupported optional param
(`UNSUPPORTED_PARAMETER`), lower `orderUpdateId` (`OUTDATED_ORDER_UPDATE`),
same id (`SAME_ORDER_UPDATE_ID`), update after cancel
(`ORDER_UPDATE_FOLLOWING_CANCEL`), start node out of range
(`START_NODE_OUT_OF_RANGE`), unreachable node
(`NODE_UNREACHABLE`/`NO_ROUTE_TO_TARGET`), wrong operating mode
(`MOBILE_ROBOT_NOT_AVAILABLE`), unknown map (`UNKNOWN_MAP_ID`), other
order active (`OTHER_ORDER_ACTIVE`).

### Actions & blocking (§6.2.2)
Actions enqueue in array order. `NONE`/`SOFT` run in parallel;
`SINGLE`/`HARD` wait for collected parallel actions to finish first.
`SOFT`/`HARD` stop automatic driving until cleared.

### Maps (§6.3)
Lifecycle via instant actions `downloadMap` → `enableMap` → `deleteMap`.
Downloaded maps appear in `state.maps` as `DISABLED` until enabled; at
most one version per `mapId` `ENABLED`. Duplicate ⇒ `DUPLICATE_MAP`
(WARNING) — fleet must delete first, then re-download.

### Request/response mechanism (§6.9)
For operations needing explicit permission (zone access, coordinated
replanning, corridor use). Robot adds a request object
(`zoneRequest`/`edgeRequest`) to its `state` with `requestStatus=REQUESTED`;
fleet answers on `responses` with `grantType`. Semantics:
- `GRANTED` (+ optional `leaseExpiry`) → may proceed until lease expires;
  fleet may extend by re-sending same `requestId` with new `leaseExpiry`.
- `QUEUED` → acknowledged, **not** permitted yet; keep waiting.
- `REJECTED` → do not proceed; may drop the request object.
- `REVOKED` / lease expiry → act per the resource's `releaseLossBehavior`.
- No response within the app's timeout → behave as **not granted**.
Remove the request from state once the operation completes/aborts.

**zoneRequest:** `{ requestId, requestType(ACCESS/REPLANNING), zoneId,
zoneSetId, requestStatus(REQUESTED/GRANTED/REVOKED/EXPIRED), *trajectory* }`.
**edgeRequest:** `{ requestId, requestType(CORRIDOR), edgeId, sequenceId,
requestStatus }` — only for base edges; robot stays on the predefined
trajectory until `GRANTED`.

### Error levels (§6.6.5.1)
- `WARNING` — may continue order, accepts new orders (may self-resolve).
- `URGENT` — needs attention; may continue & accept orders.
- `CRITICAL` — needs attention; **stops** order but **accepts** new orders.
- `FATAL` — needs user intervention; stops order and **rejects** new orders.

A robot **shall never clear its order** because of an error.

### Operating modes (§6.6.6)
| Mode | FC in control | Clear order on entry | `lastNodeId`→"" | Orders allowed | Instant actions |
|------|---------------|----------------------|-----------------|----------------|-----------------|
| `AUTOMATIC` | yes | no | no | yes | yes |
| `SEMIAUTOMATIC` | yes | no | no | yes | yes |
| `INTERVENED` | no | no | no | yes | only `cancelOrder` |
| `MANUAL` | no | yes | if continuation impossible | no | no |
| `STARTUP` | no | yes | yes | no | no |
| `SERVICE` | no | yes | yes | no | no |
| `TEACH_IN` | no | yes | yes | no | no |

Orders are only accepted in `AUTOMATIC`/`SEMIAUTOMATIC`/`INTERVENED`
(else `MOBILE_ROBOT_NOT_AVAILABLE`).

---

## J. Predefined error types (§6.6.5.4)

| errorType | Level | Trigger |
|-----------|-------|---------|
| `UNSUPPORTED_PARAMETER` | CRITICAL | Unsupported optional parameter received. |
| `NO_ORDER_TO_CANCEL` | WARNING | `cancelOrder` with no active order. |
| `VALIDATION_FAILURE` | WARNING | Malformed order. |
| `INVALID_ORDER_ACTION` | WARNING | Order with unsupported actions. |
| `INVALID_INSTANT_ACTION` | WARNING | Unsupported instant action. |
| `OUTDATED_ORDER_UPDATE` | WARNING | Correct `orderId`, outdated `orderUpdateId`. |
| `SAME_ORDER_UPDATE_ID` | WARNING | Duplicate order (same id + updateId). |
| `ORDER_UPDATE_FOLLOWING_CANCEL` | WARNING | Update for an already-cancelled order. |
| `OUTSIDE_OF_CORRIDOR` | CRITICAL | Left the edge corridor. |
| `INSUFFICIENT_MEMORY` | URGENT | Not enough memory for the order. |
| `DUPLICATE_MAP` | WARNING | `mapId`+`mapVersion` already present. |
| `BLOCKED_ZONE_VIOLATION` | CRITICAL | Entered a `BLOCKED` zone. |
| `DUPLICATE_ZONE_SET` | WARNING | `zoneSetId` already present. |
| `RELEASE_LOST` | CRITICAL | Lost release in a `RELEASE` zone. |
| `ZONE_ACTION_CONFLICT` | CRITICAL | Zone behaviour vs. zone action conflict. |
| `NODE_UNREACHABLE` | CRITICAL | Cannot reach a node in the order. |
| `LOCALIZATION_ERROR` | FATAL | Robot not localized. |
| `NO_ROUTE_TO_TARGET` | WARNING | Order has an unreachable node. |
| `OTHER_ORDER_ACTIVE` | WARNING | New order while another is active. |
| `START_NODE_OUT_OF_RANGE` | WARNING | First node unreachable. |
| `MOBILE_ROBOT_NOT_AVAILABLE` | WARNING | Order in a non-order operating mode. |
| `UNKNOWN_MAP_ID` | WARNING | Node references an unknown `mapId`. |

`errorType` is an **extensible** enum — manufacturers may add types.
Always validate concrete messages against the v3.0.0 `json_schemas/`;
where the PDF and schema differ, the **PDF/document wins**.
