#include "city.h"
#include <cstdlib>
#include <cmath>

void City::addQuad(std::vector<PBRVertex>& verts,
                   Vec3 p0, Vec3 p1, Vec3 p2, Vec3 p3,
                   Vec3 normal, Vec3 color, float metallic, float roughness) {
    verts.push_back({p0, normal, color, metallic, roughness});
    verts.push_back({p1, normal, color, metallic, roughness});
    verts.push_back({p2, normal, color, metallic, roughness});
    verts.push_back({p0, normal, color, metallic, roughness});
    verts.push_back({p2, normal, color, metallic, roughness});
    verts.push_back({p3, normal, color, metallic, roughness});
}

void City::addBox(std::vector<PBRVertex>& v, Vec3 mn, Vec3 mx, Vec3 c, float met, float rough) {
    addQuad(v, {mn.x,mx.y,mn.z},{mx.x,mx.y,mn.z},{mx.x,mx.y,mx.z},{mn.x,mx.y,mx.z}, {0,1,0}, c, met, rough);
    addQuad(v, {mn.x,mn.y,mx.z},{mx.x,mn.y,mx.z},{mx.x,mn.y,mn.z},{mn.x,mn.y,mn.z}, {0,-1,0}, c, met, rough);
    addQuad(v, {mn.x,mn.y,mx.z},{mx.x,mn.y,mx.z},{mx.x,mx.y,mx.z},{mn.x,mx.y,mx.z}, {0,0,1}, c, met, rough);
    addQuad(v, {mx.x,mn.y,mn.z},{mn.x,mn.y,mn.z},{mn.x,mx.y,mn.z},{mx.x,mx.y,mn.z}, {0,0,-1}, c, met, rough);
    addQuad(v, {mn.x,mn.y,mn.z},{mn.x,mn.y,mx.z},{mn.x,mx.y,mx.z},{mn.x,mx.y,mn.z}, {-1,0,0}, c, met, rough);
    addQuad(v, {mx.x,mn.y,mx.z},{mx.x,mn.y,mn.z},{mx.x,mx.y,mn.z},{mx.x,mx.y,mx.z}, {1,0,0}, c, met, rough);
}

void City::generateCylinder(std::vector<PBRVertex>& verts, Vec3 base, float radius,
                             float height, int segments, Vec3 color, float met, float rough) {
    for (int i = 0; i < segments; i++) {
        float a0 = 2.0f * PI * i / segments;
        float a1 = 2.0f * PI * (i + 1) / segments;
        float c0 = std::cos(a0), s0 = std::sin(a0);
        float c1 = std::cos(a1), s1 = std::sin(a1);

        Vec3 p0 = {base.x + radius*c0, base.y, base.z + radius*s0};
        Vec3 p1 = {base.x + radius*c1, base.y, base.z + radius*s1};
        Vec3 p2 = {base.x + radius*c1, base.y + height, base.z + radius*s1};
        Vec3 p3 = {base.x + radius*c0, base.y + height, base.z + radius*s0};
        Vec3 n = {(c0+c1)*0.5f, 0, (s0+s1)*0.5f};
        float nl = n.length();
        if (nl > 0.001f) n = n / nl;
        addQuad(verts, p0, p1, p2, p3, n, color, met, rough);
    }
}

void City::generateStreetLamp(std::vector<PBRVertex>& props, std::vector<PBRVertex>& emissive,
                               float x, float z, float h) {
    Vec3 poleColor = {0.3f, 0.3f, 0.32f};
    // Pole
    generateCylinder(props, {x, 0, z}, 0.06f, h, 8, poleColor, 0.5f, 0.4f);
    // Arm
    addBox(props, {x - 0.03f, h, z - 0.03f}, {x + 0.5f, h + 0.06f, z + 0.03f}, poleColor, 0.5f, 0.4f);
    // Light housing
    addBox(props, {x + 0.3f, h - 0.08f, z - 0.1f}, {x + 0.5f, h, z + 0.1f}, {0.25f, 0.25f, 0.27f}, 0.3f, 0.5f);
    // Light (emissive)
    addQuad(emissive,
            {x + 0.3f, h - 0.08f, z - 0.08f}, {x + 0.5f, h - 0.08f, z - 0.08f},
            {x + 0.5f, h - 0.08f, z + 0.08f}, {x + 0.3f, h - 0.08f, z + 0.08f},
            {0, -1, 0}, {4.0f, 3.5f, 2.5f}, 0, 0);
}

void City::generateTree(std::vector<PBRVertex>& props, float x, float z) {
    Vec3 trunk = {0.35f, 0.22f, 0.12f};
    Vec3 leaves = {0.15f + randf()*0.1f, 0.35f + randf()*0.15f, 0.1f};
    float trunkH = 1.2f + randf() * 0.5f;
    float crownR = 0.6f + randf() * 0.4f;
    float crownH = 1.5f + randf() * 0.8f;

    // Trunk
    generateCylinder(props, {x, 0, z}, 0.08f, trunkH, 6, trunk, 0, 0.9f);
    // Crown (octagonal prism approximation)
    int seg = 8;
    Vec3 crownBase = {x, trunkH, z};
    Vec3 crownTop = {x, trunkH + crownH, z};
    for (int i = 0; i < seg; i++) {
        float a0 = 2.0f * PI * i / seg;
        float a1 = 2.0f * PI * (i + 1) / seg;
        float c0 = std::cos(a0), s0 = std::sin(a0);
        float c1 = std::cos(a1), s1 = std::sin(a1);
        // Tapered: wider at middle
        float midR = crownR;
        float topR = crownR * 0.3f;
        float botR = crownR * 0.6f;

        // Bottom half
        Vec3 p0 = {x + botR*c0, trunkH, z + botR*s0};
        Vec3 p1 = {x + botR*c1, trunkH, z + botR*s1};
        Vec3 p2 = {x + midR*c1, trunkH + crownH*0.4f, z + midR*s1};
        Vec3 p3 = {x + midR*c0, trunkH + crownH*0.4f, z + midR*s0};
        Vec3 n = {(c0+c1)*0.5f, 0.3f, (s0+s1)*0.5f};
        n = n.normalized();
        addQuad(props, p0, p1, p2, p3, n, leaves, 0, 0.9f);

        // Top half
        p0 = {x + midR*c0, trunkH + crownH*0.4f, z + midR*s0};
        p1 = {x + midR*c1, trunkH + crownH*0.4f, z + midR*s1};
        p2 = {x + topR*c1, trunkH + crownH, z + topR*s1};
        p3 = {x + topR*c0, trunkH + crownH, z + topR*s0};
        n = Vec3{(c0+c1)*0.5f, 0.5f, (s0+s1)*0.5f}.normalized();
        addQuad(props, p0, p1, p2, p3, n, leaves, 0, 0.9f);
    }
}

void City::generate(int gs, float bs, float rw) {
    gridSize = gs;
    blockSize = bs;
    roadWidth = rw;
    buildings.clear();

    float totalBlock = blockSize + roadWidth;
    std::srand(42);

    for (int gx = 0; gx < gridSize; gx++) {
        for (int gz = 0; gz < gridSize; gz++) {
            float cx = (gx - gridSize / 2.0f) * totalBlock + totalBlock * 0.5f;
            float cz = (gz - gridSize / 2.0f) * totalBlock + totalBlock * 0.5f;

            // Distance from center affects building height
            float distFromCenter = std::sqrt(cx*cx + cz*cz);
            float heightMult = 1.0f + std::max(0.0f, 1.0f - distFromCenter / 40.0f) * 2.0f;

            int numB = 2 + std::rand() % 3;
            float margin = roadWidth * 0.5f;
            float avail = blockSize - margin * 2;

            for (int b = 0; b < numB; b++) {
                Building bld;
                float bw = avail * (0.25f + (std::rand() % 35) / 100.0f);
                float bd = avail * (0.25f + (std::rand() % 35) / 100.0f);
                float bh = (3.0f + (std::rand() % 180) / 10.0f) * heightMult;

                float ox = (std::rand() % 100 / 100.0f) * (avail - bw) - avail*0.5f + bw*0.5f;
                float oz = (std::rand() % 100 / 100.0f) * (avail - bd) - avail*0.5f + bd*0.5f;

                bld.position = {cx + ox, 0, cz + oz};
                bld.width = bw;
                bld.depth = bd;
                bld.height = bh;
                bld.style = std::rand() % 5;

                switch (bld.style) {
                    case 0: // Office — concrete
                        bld.color = {0.72f + randf()*0.08f, 0.70f + randf()*0.08f, 0.68f + randf()*0.08f};
                        bld.metallic = 0.05f; bld.roughness = 0.8f; break;
                    case 1: // Residential — warm tones
                        bld.color = {0.75f + randf()*0.1f, 0.55f + randf()*0.1f, 0.40f + randf()*0.1f};
                        bld.metallic = 0.0f; bld.roughness = 0.9f; break;
                    case 2: // Glass tower
                        bld.color = {0.30f + randf()*0.1f, 0.35f + randf()*0.1f, 0.42f + randf()*0.1f};
                        bld.metallic = 0.6f; bld.roughness = 0.1f; break;
                    case 3: // Brick
                        bld.color = {0.55f + randf()*0.1f, 0.28f + randf()*0.08f, 0.18f + randf()*0.08f};
                        bld.metallic = 0.0f; bld.roughness = 0.95f; break;
                    case 4: // Modern — dark
                        bld.color = {0.20f + randf()*0.1f, 0.22f + randf()*0.1f, 0.25f + randf()*0.1f};
                        bld.metallic = 0.7f; bld.roughness = 0.15f; break;
                }
                buildings.push_back(bld);
            }
        }
    }
}

void City::buildMesh() {
    float totalBlock = blockSize + roadWidth;
    float halfCity = gridSize * totalBlock * 0.5f + roadWidth;

    std::vector<PBRVertex> groundV, roadV, sidewalkV, buildV, emissiveV, propV;

    // ── Ground (asphalt) ──
    addQuad(groundV,
            {-halfCity, 0.0f, -halfCity}, {halfCity, 0.0f, -halfCity},
            {halfCity, 0.0f, halfCity}, {-halfCity, 0.0f, halfCity},
            {0,1,0}, {0.16f, 0.16f, 0.18f}, 0.0f, 0.85f);

    // ── Road markings ──
    Vec3 yellow = {0.95f, 0.85f, 0.15f};
    Vec3 white  = {0.9f, 0.9f, 0.9f};

    for (int i = -gridSize/2 - 1; i <= gridSize/2 + 1; i++) {
        float pos = i * totalBlock;
        // Dashed center lines
        for (float t = -halfCity; t < halfCity; t += 3.0f) {
            addQuad(roadV, {t,0.015f,pos-0.06f},{t+1.5f,0.015f,pos-0.06f},
                    {t+1.5f,0.015f,pos+0.06f},{t,0.015f,pos+0.06f}, {0,1,0}, yellow, 0, 0.9f);
            addQuad(roadV, {pos-0.06f,0.015f,t},{pos+0.06f,0.015f,t},
                    {pos+0.06f,0.015f,t+1.5f},{pos-0.06f,0.015f,t+1.5f}, {0,1,0}, yellow, 0, 0.9f);
        }
        // Edge lines
        float eo = roadWidth * 0.45f;
        addQuad(roadV, {-halfCity,0.012f,pos+eo},{halfCity,0.012f,pos+eo},
                {halfCity,0.012f,pos+eo+0.08f},{-halfCity,0.012f,pos+eo+0.08f}, {0,1,0}, white, 0, 0.7f);
        addQuad(roadV, {-halfCity,0.012f,pos-eo-0.08f},{halfCity,0.012f,pos-eo-0.08f},
                {halfCity,0.012f,pos-eo},{-halfCity,0.012f,pos-eo}, {0,1,0}, white, 0, 0.7f);
        addQuad(roadV, {pos+eo,0.012f,-halfCity},{pos+eo+0.08f,0.012f,-halfCity},
                {pos+eo+0.08f,0.012f,halfCity},{pos+eo,0.012f,halfCity}, {0,1,0}, white, 0, 0.7f);
        addQuad(roadV, {pos-eo-0.08f,0.012f,-halfCity},{pos-eo,0.012f,-halfCity},
                {pos-eo,0.012f,halfCity},{pos-eo-0.08f,0.012f,halfCity}, {0,1,0}, white, 0, 0.7f);

        // Crosswalks at intersections
        for (int j = -gridSize/2 - 1; j <= gridSize/2 + 1; j++) {
            float ipos = j * totalBlock;
            // Zebra stripes
            for (float s = -roadWidth*0.4f; s < roadWidth*0.4f; s += 0.5f) {
                addQuad(roadV,
                        {pos + s, 0.013f, ipos - roadWidth*0.45f},
                        {pos + s + 0.25f, 0.013f, ipos - roadWidth*0.45f},
                        {pos + s + 0.25f, 0.013f, ipos - roadWidth*0.15f},
                        {pos + s, 0.013f, ipos - roadWidth*0.15f},
                        {0,1,0}, white, 0, 0.7f);
                addQuad(roadV,
                        {ipos - roadWidth*0.45f, 0.013f, pos + s},
                        {ipos - roadWidth*0.15f, 0.013f, pos + s},
                        {ipos - roadWidth*0.15f, 0.013f, pos + s + 0.25f},
                        {ipos - roadWidth*0.45f, 0.013f, pos + s + 0.25f},
                        {0,1,0}, white, 0, 0.7f);
            }
        }
    }

    // ── Sidewalks + street furniture ──
    Vec3 concreteColor = {0.62f, 0.60f, 0.57f};
    float swH = 0.15f;
    float swW = roadWidth * 0.18f;

    for (int gx = 0; gx < gridSize; gx++) {
        for (int gz = 0; gz < gridSize; gz++) {
            float cx = (gx - gridSize/2.0f) * totalBlock + totalBlock*0.5f;
            float cz = (gz - gridSize/2.0f) * totalBlock + totalBlock*0.5f;
            float hbs = blockSize * 0.5f;

            // Sidewalk curbs (4 sides)
            // Front
            addQuad(sidewalkV, {cx-hbs,0,cz+hbs},{cx+hbs,0,cz+hbs},
                    {cx+hbs,swH,cz+hbs},{cx-hbs,swH,cz+hbs}, {0,0,1}, concreteColor, 0, 0.85f);
            addQuad(sidewalkV, {cx-hbs,swH,cz+hbs},{cx+hbs,swH,cz+hbs},
                    {cx+hbs,swH,cz+hbs+swW},{cx-hbs,swH,cz+hbs+swW}, {0,1,0}, concreteColor, 0, 0.85f);
            // Back
            addQuad(sidewalkV, {cx+hbs,0,cz-hbs},{cx-hbs,0,cz-hbs},
                    {cx-hbs,swH,cz-hbs},{cx+hbs,swH,cz-hbs}, {0,0,-1}, concreteColor, 0, 0.85f);
            addQuad(sidewalkV, {cx-hbs,swH,cz-hbs-swW},{cx+hbs,swH,cz-hbs-swW},
                    {cx+hbs,swH,cz-hbs},{cx-hbs,swH,cz-hbs}, {0,1,0}, concreteColor, 0, 0.85f);
            // Left
            addQuad(sidewalkV, {cx-hbs,0,cz-hbs},{cx-hbs,0,cz+hbs},
                    {cx-hbs,swH,cz+hbs},{cx-hbs,swH,cz-hbs}, {-1,0,0}, concreteColor, 0, 0.85f);
            addQuad(sidewalkV, {cx-hbs-swW,swH,cz-hbs},{cx-hbs,swH,cz-hbs},
                    {cx-hbs,swH,cz+hbs},{cx-hbs-swW,swH,cz+hbs}, {0,1,0}, concreteColor, 0, 0.85f);
            // Right
            addQuad(sidewalkV, {cx+hbs,0,cz+hbs},{cx+hbs,0,cz-hbs},
                    {cx+hbs,swH,cz-hbs},{cx+hbs,swH,cz+hbs}, {1,0,0}, concreteColor, 0, 0.85f);
            addQuad(sidewalkV, {cx+hbs,swH,cz-hbs},{cx+hbs+swW,swH,cz-hbs},
                    {cx+hbs+swW,swH,cz+hbs},{cx+hbs,swH,cz+hbs}, {0,1,0}, concreteColor, 0, 0.85f);

            // Street lamps along sidewalks
            float lampSpacing = 8.0f;
            for (float lz = cz - hbs; lz <= cz + hbs; lz += lampSpacing) {
                generateStreetLamp(propV, emissiveV, cx - hbs - swW*0.5f, lz, 3.5f);
                generateStreetLamp(propV, emissiveV, cx + hbs + swW*0.5f, lz, 3.5f);
            }

            // Trees along sidewalks
            float treeSpacing = 6.0f;
            for (float tz = cz - hbs + 2.0f; tz <= cz + hbs - 2.0f; tz += treeSpacing) {
                if (std::rand() % 3 != 0) {
                    generateTree(propV, cx - hbs - swW*0.3f, tz + randf(-0.5f, 0.5f));
                }
                if (std::rand() % 3 != 0) {
                    generateTree(propV, cx + hbs + swW*0.3f, tz + randf(-0.5f, 0.5f));
                }
            }

            // Fire hydrants (occasional)
            if (std::rand() % 4 == 0) {
                float hx = cx + hbs + swW * 0.4f;
                float hz = cz + randf(-hbs*0.5f, hbs*0.5f);
                generateCylinder(propV, {hx, swH, hz}, 0.06f, 0.4f, 6, {0.8f, 0.15f, 0.1f}, 0.1f, 0.7f);
                addBox(propV, {hx-0.08f, swH+0.25f, hz-0.04f}, {hx+0.08f, swH+0.35f, hz+0.04f},
                       {0.8f, 0.15f, 0.1f}, 0.1f, 0.7f);
            }
        }
    }

    // ── Buildings ──
    for (auto& b : buildings) {
        float hw = b.width*0.5f, hd = b.depth*0.5f, h = b.height;
        float x = b.position.x, z = b.position.z;
        Vec3 c = b.color;
        float met = b.metallic, rough = b.roughness;
        Vec3 cDark = c * 0.75f;
        Vec3 cTop = c * 0.9f;

        // Main faces
        addQuad(buildV, {x-hw,h,z-hd},{x+hw,h,z-hd},{x+hw,h,z+hd},{x-hw,h,z+hd}, {0,1,0}, cTop, met, rough);
        addQuad(buildV, {x-hw,0,z+hd},{x+hw,0,z+hd},{x+hw,h,z+hd},{x-hw,h,z+hd}, {0,0,1}, c, met, rough);
        addQuad(buildV, {x+hw,0,z-hd},{x-hw,0,z-hd},{x-hw,h,z-hd},{x+hw,h,z-hd}, {0,0,-1}, cDark, met, rough);
        addQuad(buildV, {x-hw,0,z-hd},{x-hw,0,z+hd},{x-hw,h,z+hd},{x-hw,h,z-hd}, {-1,0,0}, cDark, met, rough);
        addQuad(buildV, {x+hw,0,z+hd},{x+hw,0,z-hd},{x+hw,h,z-hd},{x+hw,h,z+hd}, {1,0,0}, c, met, rough);

        // Ledges every few floors
        float ledgeDepth = 0.08f;
        for (float ly = 3.0f; ly < h - 1.0f; ly += 3.5f) {
            addBox(buildV, {x-hw-ledgeDepth, ly, z-hd-ledgeDepth},
                   {x+hw+ledgeDepth, ly+0.12f, z+hd+ledgeDepth}, c*0.85f, met, rough+0.1f);
        }

        // Ground floor darker base
        float baseH = std::min(2.5f, h * 0.15f);
        addQuad(buildV, {x-hw-0.01f,0,z+hd+0.01f},{x+hw+0.01f,0,z+hd+0.01f},
                {x+hw+0.01f,baseH,z+hd+0.01f},{x-hw-0.01f,baseH,z+hd+0.01f},
                {0,0,1}, c*0.5f, 0, 0.9f);
        addQuad(buildV, {x+hw+0.01f,0,z-hd-0.01f},{x-hw-0.01f,0,z-hd-0.01f},
                {x-hw-0.01f,baseH,z-hd-0.01f},{x+hw+0.01f,baseH,z-hd-0.01f},
                {0,0,-1}, c*0.5f, 0, 0.9f);

        // Windows (emissive)
        float winW = 0.5f, winH = 0.7f, winSpacingX = 1.1f, winSpacingY = 2.0f;
        if (b.style == 2) { winW = 0.8f; winH = 1.5f; winSpacingX = 1.0f; winSpacingY = 1.8f; }

        for (float wy = baseH + 0.8f; wy + winH < h - 0.5f; wy += winSpacingY) {
            // Front
            for (float wx = x - hw + 0.4f; wx + winW < x + hw - 0.2f; wx += winSpacingX) {
                bool lit = (std::rand() % 3) != 0;
                Vec3 wc = lit ? Vec3{1.2f, 1.05f, 0.75f} : Vec3{0.5f, 0.6f, 0.75f};
                addQuad(emissiveV, {wx,wy,z+hd+0.015f},{wx+winW,wy,z+hd+0.015f},
                        {wx+winW,wy+winH,z+hd+0.015f},{wx,wy+winH,z+hd+0.015f},
                        {0,0,1}, wc, 0, 0.1f);
            }
            // Back
            for (float wx = x - hw + 0.4f; wx + winW < x + hw - 0.2f; wx += winSpacingX) {
                bool lit = (std::rand() % 3) != 0;
                Vec3 wc = lit ? Vec3{1.2f, 1.05f, 0.75f} : Vec3{0.5f, 0.6f, 0.75f};
                addQuad(emissiveV, {wx+winW,wy,z-hd-0.015f},{wx,wy,z-hd-0.015f},
                        {wx,wy+winH,z-hd-0.015f},{wx+winW,wy+winH,z-hd-0.015f},
                        {0,0,-1}, wc, 0, 0.1f);
            }
            // Left
            for (float wz = z - hd + 0.4f; wz + winW < z + hd - 0.2f; wz += winSpacingX) {
                bool lit = (std::rand() % 3) != 0;
                Vec3 wc = lit ? Vec3{1.2f, 1.05f, 0.75f} : Vec3{0.5f, 0.6f, 0.75f};
                addQuad(emissiveV, {x-hw-0.015f,wy,wz},{x-hw-0.015f,wy,wz+winW},
                        {x-hw-0.015f,wy+winH,wz+winW},{x-hw-0.015f,wy+winH,wz},
                        {-1,0,0}, wc, 0, 0.1f);
            }
            // Right
            for (float wz = z - hd + 0.4f; wz + winW < z + hd - 0.2f; wz += winSpacingX) {
                bool lit = (std::rand() % 3) != 0;
                Vec3 wc = lit ? Vec3{1.2f, 1.05f, 0.75f} : Vec3{0.5f, 0.6f, 0.75f};
                addQuad(emissiveV, {x+hw+0.015f,wy,wz+winW},{x+hw+0.015f,wy,wz},
                        {x+hw+0.015f,wy+winH,wz},{x+hw+0.015f,wy+winH,wz+winW},
                        {1,0,0}, wc, 0, 0.1f);
            }
        }

        // Rooftop details
        if (h > 6.0f) {
            // AC units
            int nAC = 1 + std::rand() % 3;
            for (int a = 0; a < nAC; a++) {
                float ax = x + randf(-hw*0.4f, hw*0.4f);
                float az = z + randf(-hd*0.4f, hd*0.4f);
                addBox(buildV, {ax-0.3f, h, az-0.3f}, {ax+0.3f, h+0.25f, az+0.3f},
                       {0.5f, 0.5f, 0.52f}, 0.3f, 0.6f);
            }
            // Antenna/spire on tall buildings
            if (h > 12.0f && std::rand() % 3 == 0) {
                float spireH = h * 0.15f;
                generateCylinder(buildV, {x, h, z}, 0.04f, spireH, 6, {0.6f, 0.6f, 0.62f}, 0.7f, 0.3f);
                // Blinking light on top
                addBox(emissiveV, {x-0.05f, h+spireH, z-0.05f}, {x+0.05f, h+spireH+0.06f, z+0.05f},
                       {3.0f, 0.2f, 0.1f}, 0, 0);
            }
        }
    }

    // Upload all meshes
    groundMesh.upload(groundV);
    roadMarkingMesh.upload(roadV);
    sidewalkMesh.upload(sidewalkV);
    buildingMesh.upload(buildV);
    emissiveMesh.upload(emissiveV);
    propMesh.upload(propV);

    std::printf("City: %d ground + %d road + %d sidewalk + %d building + %d emissive + %d prop verts\n",
                (int)groundV.size(), (int)roadV.size(), (int)sidewalkV.size(),
                (int)buildV.size(), (int)emissiveV.size(), (int)propV.size());
}

void City::draw(const Shader& shader, const Mat4& view, const Mat4& proj) const {
    Mat4 model;
    model.identity();
    shader.use();
    shader.setMat4("model", model.data());
    shader.setMat4("view", view.data());
    shader.setMat4("projection", proj.data());
    shader.setFloat("emissive", 0.0f);
    shader.setFloat("draftGlow", 0.0f);

    groundMesh.draw();
    roadMarkingMesh.draw();
    sidewalkMesh.draw();
    buildingMesh.draw();
    propMesh.draw();
}

void City::drawEmissive(const Shader& shader, const Mat4& view, const Mat4& proj) const {
    Mat4 model;
    model.identity();
    shader.use();
    shader.setMat4("model", model.data());
    shader.setMat4("view", view.data());
    shader.setMat4("projection", proj.data());
    shader.setFloat("emissive", 1.0f);
    shader.setFloat("draftGlow", 0.0f);

    emissiveMesh.draw();
}

void City::drawShadow(const Shader& shader, const Mat4& lightSpace) const {
    Mat4 model;
    model.identity();
    shader.use();
    shader.setMat4("model", model.data());
    shader.setMat4("lightSpaceMatrix", lightSpace.data());

    buildingMesh.draw();
    propMesh.draw();
}

bool City::collides(const Vec3& pos, float radius) const {
    for (auto& b : buildings) {
        float hw = b.width*0.5f + radius;
        float hd = b.depth*0.5f + radius;
        if (std::fabs(pos.x - b.position.x) < hw && std::fabs(pos.z - b.position.z) < hd)
            return true;
    }
    return false;
}

Vec3 City::pushOut(const Vec3& pos, float radius) const {
    Vec3 result = pos;
    for (auto& b : buildings) {
        float hw = b.width*0.5f + radius;
        float hd = b.depth*0.5f + radius;
        float dx = result.x - b.position.x;
        float dz = result.z - b.position.z;
        if (std::fabs(dx) < hw && std::fabs(dz) < hd) {
            float overlapX = hw - std::fabs(dx);
            float overlapZ = hd - std::fabs(dz);
            if (overlapX < overlapZ)
                result.x += (dx > 0 ? overlapX : -overlapX);
            else
                result.z += (dz > 0 ? overlapZ : -overlapZ);
        }
    }
    return result;
}
