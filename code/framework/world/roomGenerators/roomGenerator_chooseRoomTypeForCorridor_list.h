
// although doorA and doorB are outbound we turn them around here
VAR(Range, absAngleTowards); // angle towards doorB from doorA
VAR(Range, absAngle); // angle between both door (turned around) (0 - same dir, 180 facing each other)
VAR(Range, absX); // doorB loc relative to doorA (turned around)
VAR(Range, x); // doorB loc relative to doorA (turned around)
VAR(Range, y); // doorB loc relative to doorA (turned around)
VAR(Range, dist); // distance in doorA's xy plane
