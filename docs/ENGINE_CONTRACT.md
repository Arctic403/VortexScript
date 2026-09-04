# Vortex Engine Compatibility Contract Snapshot

This repository does not modify or depend on the Vortex3D repository. The following compatibility rules are copied into VortexScript so the frontend can remain standalone and integration-ready.

## Contract version

Current snapshot: `1`.

A host-provided command schema must advertise the same contract version or compilation fails.

## Persistent identity

- durable Vortex identities are represented as fixed-width 64-bit unsigned values;
- zero is invalid;
- kinds remain explicit (`Object`, `Mesh`, `Vertex`, etc.);
- IDs are not array positions or pointers.

VortexScript therefore retains entity kind + 64-bit value in planned entity references.

## Coordinate contract

VortexScript's compatibility snapshot is:

- right handed;
- +X right;
- +Y forward;
- +Z up;
- meters;
- radians;
- column-major matrices;
- column-vector convention;
- quaternion component storage `x,y,z,w`.

The frontend does not currently perform transform math. These constants prevent future vector/quaternion syntax from silently choosing a conflicting convention.

## Mutation contract

VortexScript plans are descriptions of requested commands. They do not mutate persistent state themselves. A host adapter must execute persistent work through its normal command/transaction path so failed transactions remain atomic and editor/script/AI automation share the same authority model.

## glTF boundary

glTF is interchange, not VortexScript's persistent authoring model. glTF 2.0 uses right-handed coordinates, +Y up, +Z forward, meters, and radians, so import/export adapters must perform an explicit basis conversion rather than treating values as already-native Vortex coordinates.
