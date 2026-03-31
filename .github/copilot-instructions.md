# Copilot Instructions

## Project Guidelines
- Use non-inverted vertical mouse look for camera controls.
- For transforms, prefer setter/getter-based local/world synchronization with lazy stale computation; propagation should resolve hierarchy-wide staleness rather than being the only way world state updates.