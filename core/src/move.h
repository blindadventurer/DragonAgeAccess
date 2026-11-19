#pragma once

// Orders the creature with this object id (the party leader: Party.Leader.ID) to
// walk to world x, y through the game's own click-to-move path. playerX/Y are the
// leader's current position, used once to find where the object keeps its x, y, z.
// Game thread only. Returns false when refused or when the call faulted.
bool MoveCreatureTo(unsigned id, double x, double y, double playerX, double playerY);
