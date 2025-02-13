#include <cmath>
#include <fstream>
#include <raylib.h>
#include <string>
#include <vector>

constexpr int W = 1280;
constexpr int H = 720;
struct Particle {
    Vector2 position;
    Vector3 velocity;
    Color color;
    float mass, r;
};

struct Surface {
    Vector2 p1, p2;
};
float dot(Vector2 a, Vector2 b) {
    return std::sqrt(a.x * b.x + a.y * b.y);
}
float magnitude(Vector2 v) { return std::sqrt(v.x * v.x + v.y * v.y); }
float magnitude2(Vector3 v) { return std::sqrt(v.x * v.x + v.y * v.y); }

Vector2 normalize(Vector2 v) {
    float mag = magnitude(v);
    if (mag == 0) return {0, 0};
    return {v.x / mag, v.y / mag};
}

Vector3 normalize2(Vector3 v) {
    float mag = magnitude2(v);
    if (mag == 0) return {0, 0, 0};
    return {v.x / mag, v.y / mag, v.z};
}

struct Scene {
    const bool bouncyBorders = false;
    const Vector2 defaultVel = {0, 0};
    const std::vector<Particle> particles = {};
    const std::vector<Surface> surfaces   = {};
};

Scene Load(std::string file) {
    std::ifstream ifs(file);
    bool bouncyBorders;
    Vector2 defaultVel;

    ifs.read((char*)&bouncyBorders, sizeof(bool));
    ifs.read((char*)&defaultVel, sizeof(Vector2));

    size_t size;
    ifs.read((char*)&size, sizeof(typeof(size)));
    
    std::vector<Particle> particles(size);
    ifs.read((char*)particles.data(), sizeof(Particle) * size);

    ifs.read((char*)&size, sizeof(typeof(size)));
    std::vector<Surface> surfaces(size);
    ifs.read((char*)surfaces.data(), sizeof(Surface) * size);
    return {
        .bouncyBorders = bouncyBorders,
        .defaultVel = defaultVel,
        .particles = particles,
        .surfaces = surfaces
    };
}

void Save(std::string file, const Scene& scene) {
    std::ofstream ofs(file);
    ofs.write((char*)&scene.bouncyBorders, sizeof(bool));
    ofs.write((char*)&scene.defaultVel, sizeof(Vector2));

    typeof(scene.particles.size()) size = scene.particles.size();
    ofs.write((char*)&size, sizeof(typeof(size)));
    ofs.write((char*)scene.particles.data(), sizeof(Particle) * size);

    size = scene.surfaces.size();
    ofs.write((char*)&size, sizeof(typeof(size)));
    ofs.write((char*)scene.surfaces.data(), sizeof(Surface) * size);
    ofs.close();
}


void Copy(Vector3& dst, Vector2 src) {
    dst.x = src.x;
    dst.y = src.y;
}

void Simulate(const Scene& scene) {
    Vector2 actualDefault = normalize(scene.defaultVel);
    std::vector<Surface> surfaces = scene.surfaces;
    std::vector<Particle> particles = scene.particles;
    float max = 1;
    for(int i =0; i < particles.size(); i++) {
        particles[i].velocity = normalize2(particles[i].velocity);
        if(particles[i].velocity.z > max) max = particles[i].velocity.z;
    }
    SetWindowTitle("OptiX Simulation");
    SetTargetFPS(60 * max);
    
    for(Particle& particle : particles) {
        particle.velocity.z /= max;
    }
    
    // Running simulation
    while(!WindowShouldClose()) {
        std::string speed = "1 / " + std::to_string(max) + "x";
        DrawText(speed.c_str(), 0, 0, 25, RED);
        
        for(int i =0; i < particles.size(); i++) {
            Vector2& pos = particles[i].position;
            pos.x += particles[i].velocity.x * particles[i].velocity.z;
            pos.y += particles[i].velocity.y * particles[i].velocity.z;

            Copy(particles[i].velocity, normalize({
                    particles[i].velocity.x + scene.defaultVel.x,
                    particles[i].velocity.y + scene.defaultVel.y,
                    }));
            
            for(Surface surface : surfaces) {
                if(CheckCollisionCircleLine(particles[i].position, particles[i].r, surface.p1, surface.p2)) {
                    Vector2 Line = normalize({surface.p2.x - surface.p1.x, surface.p2.y - surface.p1.y});
                    Vector2 Part = normalize({particles[i].velocity.x, particles[i].velocity.y});
                    float LineAngle = atan2f(Line.y, Line.x);
                    float PartAngle = atan2f(Part.y, Part.x);
                    float OtherAngle = 2 * LineAngle - PartAngle;
                    
                    Copy(particles[i].velocity, normalize({cosf(OtherAngle), sinf(OtherAngle)}));
                }
            }
        }
        
        BeginDrawing();
        ClearBackground(BLACK);
        for(Surface surface : surfaces) DrawLineV(surface.p1, surface.p2, WHITE);
        for(Particle p : particles) DrawCircleV(p.position, p.r, p.color);
        EndDrawing();
    }
}

void Design() {
    SetTargetFPS(60);
    InitWindow(W, H, "OptiX Designer");
    int selected = 0;
    Vector2 start;
    std::vector<Surface> surfaces;
    std::vector<Particle> particles;
    while(!WindowShouldClose()) {
        Vector2 mouse = GetMousePosition();
        BeginDrawing();
        ClearBackground(BLACK);
        
        DrawLineV({0, 0}, {20, 20}, WHITE);
        DrawCircleV({30, 10}, (selected)? 5 : 10, WHITE);
        DrawRectangleLinesEx({(float)selected * 20, 0, 20, 20}, 3, RED);

        for(Surface surface : surfaces) DrawLineV(surface.p1, surface.p2, WHITE);
        for(Particle p : particles) {
            DrawCircleV(p.position, p.r, p.color);
            DrawLineEx(p.position,  {p.position.x + p.velocity.x * p.velocity.z, p.position.y + p.velocity.y * p.velocity.z}, 3, BLUE);
        }
        
        if(IsKeyPressed(KEY_ONE)) selected = 0;
        if(IsKeyPressed(KEY_TWO)) selected = 1;
        if(IsKeyPressed(KEY_S)) {
            Save("scene.sc", (Scene){false, {0, 0}, particles, surfaces });
            EndDrawing();
            
            Simulate((Scene){false, {0, 0}, particles, surfaces });
            SetTargetFPS(60);
            InitWindow(W, H, "OptiX Designer");
        }
        
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            start = mouse;
        
        if(selected == 0) {
            if(IsMouseButtonDown(MOUSE_BUTTON_LEFT))
                DrawLineV(start, mouse, WHITE);
            if(IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
                surfaces.push_back({start, mouse});
        } else if(selected == 1) {
            float dst = magnitude({ mouse.x - start.x, mouse.y - start.y, });
            if(IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                DrawCircleV(start, dst, RED);
            }
            if(IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                if(dst == 0) {
                    EndDrawing();
                    continue;
                }
                particles.push_back((Particle){start, {}, RED, 1, dst});
                selected = 2;
            }
        } else if(selected == 2) {
            DrawLineEx(start, mouse, 3, BLUE);
            Particle& setting = particles[particles.size() - 1];
            if(IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                float dst = magnitude({ mouse.x - setting.position.x, mouse.y - setting.position.y, });
                Vector2 vel = normalize({ mouse.x - setting.position.x, mouse.y - setting.position.y});
                setting.velocity = { vel.x, vel.y, dst, };
                selected = 1;
            }
        }
        
        EndDrawing();
    }
}

int main(int argc, char** argv) {    
    SetTraceLogLevel(LOG_NONE);

    std::string file = (argc >= 2)?argv[1] : "file.sc";
    Design();
    Simulate(Load("scene.sc"));
}
