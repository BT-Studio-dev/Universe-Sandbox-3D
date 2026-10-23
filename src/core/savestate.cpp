// ============================================================================
//  Universe Sandbox 3D - save / load (implementation)
// ============================================================================
#include "core/savestate.h"

#include <cstdio>
#include <fstream>
#include <sstream>

namespace us3d {

static std::string quote(const std::string& s) {
    std::string r = "\"";
    for (char c : s) {
        if (c == '"' || c == '\\') r.push_back('\\');
        r.push_back(c);
    }
    r.push_back('"');
    return r;
}

// Reads a double-quoted token from the stream, honoring embedded spaces and
// backslash escapes (names like "Old Sun" must survive a round trip).
static bool readQuoted(std::istream& f, std::string& out) {
    f >> std::ws;
    if (f.peek() != '"') return false;
    f.get(); // opening quote
    out.clear();
    int c;
    while ((c = f.get()) != EOF) {
        if (c == '\\') {
            c = f.get();
            if (c == EOF) break;
        } else if (c == '"') {
            return true;
        }
        out.push_back((char)c);
    }
    return false;
}

SaveResult saveSystem(const System& s, const std::string& path) {
    SaveResult r;
    std::ofstream f(path.c_str());
    if (!f) { r.message = "Could not open file for writing."; return r; }

    f << "US3D SAVE 2\n";
    f << "time " << s.time() << "\n";
    f << "reversed " << (s.reversed() ? 1 : 0) << "\n";

    for (const Body& b : s.bodies()) {
        if (b.dead) continue;
        f << "body " << quote(b.name) << " " << (int)b.type << " "
          << b.pos.x << " " << b.pos.y << " " << b.pos.z << " "
          << b.vel.x << " " << b.vel.y << " " << b.vel.z << " "
          << b.mass << " " << b.radius << " "
          << b.color.x << " " << b.color.y << " " << b.color.z << " "
          << b.bandFreq << " "
          << b.realRadiusAU << " " << b.surface << " "
          << b.temperatureC << " " << b.spin << "\n";
    }
    f.close();
    if (f.fail()) { r.message = "Write failed."; return r; }

    char buf[128];
    std::snprintf(buf, sizeof buf, "Saved %d bodies to %s", s.bodyCount(), path.c_str());
    r.ok = true;
    r.message = buf;
    return r;
}

SaveResult loadSystem(System& s, const std::string& path) {
    SaveResult r;
    std::ifstream f(path.c_str());
    if (!f) { r.message = "Could not open file for reading."; return r; }

    std::string magic, tag, ver;
    f >> magic >> tag >> ver;
    if (magic != "US3D" || tag != "SAVE" ||
        (ver != "1" && ver != "2")) {
        r.message = "Not a valid US3D save file (v1/v2).";
        return r;
    }
    const bool v2 = (ver == "2");

    s.clearAll();

    std::string key;
    while (f >> key) {
        if (key == "time") {
            float t; f >> t; s.setTime(t);
        } else if (key == "reversed") {
            int v; f >> v; s.setReversed(v != 0);
        } else if (key == "body") {
            std::string name;
            if (!readQuoted(f, name)) name = "Body";
            int type = 0;
            f >> type;
            Body b;
            b.name = name;
            b.type = (BodyType)type;
            f >> b.pos.x >> b.pos.y >> b.pos.z;
            f >> b.vel.x >> b.vel.y >> b.vel.z;
            f >> b.mass >> b.radius;
            f >> b.color.x >> b.color.y >> b.color.z;
            f >> b.bandFreq;
            if (v2) {
                int surface = 0;
                f >> b.realRadiusAU >> surface >> b.temperatureC >> b.spin;
                b.surface = surface;
            }
            s.addBody(b);
        }
    }

    char buf[160];
    std::snprintf(buf, sizeof buf, "Loaded %d bodies from %s",
                  s.bodyCount(), path.c_str());
    r.ok = s.bodyCount() > 0;
    r.message = r.ok ? buf : "File contained no bodies.";
    return r;
}

} // namespace us3d
