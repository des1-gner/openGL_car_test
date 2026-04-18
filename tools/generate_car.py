#!/usr/bin/env python3
"""
Generate a smooth sports car OBJ model using spline-lofted cross sections.
Outputs: assets/car_body.obj, assets/car_glass.obj, assets/car_wheels.obj, assets/car_lights.obj
"""
import math
import os

os.makedirs("assets", exist_ok=True)

def lerp(a, b, t):
    return a + (b - a) * t

def smoothstep(t):
    return t * t * (3 - 2 * t)

def catmull_rom(p0, p1, p2, p3, t):
    """Catmull-Rom spline interpolation."""
    t2 = t * t
    t3 = t2 * t
    return 0.5 * (
        (2 * p1) +
        (-p0 + p2) * t +
        (2*p0 - 5*p1 + 4*p2 - p3) * t2 +
        (-p0 + 3*p1 - 3*p2 + p3) * t3
    )

class OBJWriter:
    def __init__(self):
        self.verts = []
        self.normals = []
        self.faces = []

    def add_vert(self, x, y, z):
        self.verts.append((x, y, z))
        return len(self.verts)

    def add_normal(self, nx, ny, nz):
        l = math.sqrt(nx*nx + ny*ny + nz*nz)
        if l > 1e-7:
            nx, ny, nz = nx/l, ny/l, nz/l
        self.normals.append((nx, ny, nz))
        return len(self.normals)

    def add_face(self, v_indices, n_indices=None):
        """Add a face. Indices are 1-based."""
        if n_indices:
            self.faces.append(list(zip(v_indices, n_indices)))
        else:
            self.faces.append([(v,) for v in v_indices])

    def add_quad_smooth(self, p0, p1, p2, p3):
        """Add a quad with per-vertex normals computed from adjacent faces."""
        # Compute face normal
        e1 = (p1[0]-p0[0], p1[1]-p0[1], p1[2]-p0[2])
        e2 = (p3[0]-p0[0], p3[1]-p0[1], p3[2]-p0[2])
        nx = e1[1]*e2[2] - e1[2]*e2[1]
        ny = e1[2]*e2[0] - e1[0]*e2[2]
        nz = e1[0]*e2[1] - e1[1]*e2[0]

        vi0 = self.add_vert(*p0)
        vi1 = self.add_vert(*p1)
        vi2 = self.add_vert(*p2)
        vi3 = self.add_vert(*p3)
        ni = self.add_normal(nx, ny, nz)
        self.add_face([vi0, vi1, vi2, vi3], [ni, ni, ni, ni])

    def add_tri(self, p0, p1, p2):
        e1 = (p1[0]-p0[0], p1[1]-p0[1], p1[2]-p0[2])
        e2 = (p2[0]-p0[0], p2[1]-p0[1], p2[2]-p0[2])
        nx = e1[1]*e2[2] - e1[2]*e2[1]
        ny = e1[2]*e2[0] - e1[0]*e2[2]
        nz = e1[0]*e2[1] - e1[1]*e2[0]
        vi0 = self.add_vert(*p0)
        vi1 = self.add_vert(*p1)
        vi2 = self.add_vert(*p2)
        ni = self.add_normal(nx, ny, nz)
        self.add_face([vi0, vi1, vi2], [ni, ni, ni])

    def save(self, path):
        with open(path, 'w') as f:
            f.write(f"# Generated car model - {len(self.verts)} vertices\n")
            for v in self.verts:
                f.write(f"v {v[0]:.6f} {v[1]:.6f} {v[2]:.6f}\n")
            for n in self.normals:
                f.write(f"vn {n[0]:.6f} {n[1]:.6f} {n[2]:.6f}\n")
            for face in self.faces:
                parts = []
                for entry in face:
                    if len(entry) == 2:
                        parts.append(f"{entry[0]}//{entry[1]}")
                    else:
                        parts.append(f"{entry[0]}")
                f.write("f " + " ".join(parts) + "\n")
        print(f"Saved {path}: {len(self.verts)} verts, {len(self.faces)} faces")


# ═══════════════════════════════════════════════════════════════════════════════
# Car body profile — defines cross-section at each Z station
# ═══════════════════════════════════════════════════════════════════════════════

# Car dimensions (in game units, Z = forward)
CAR_LENGTH = 2.2
CAR_HALF_LENGTH = CAR_LENGTH / 2

# Cross-section: (z_normalized [0=rear, 1=front], half_width, floor_y, body_top_y, belt_y, roof_half_width, roof_y)
# z_normalized maps to [-half_length, +half_length]
raw_profiles = [
    # z_norm, half_w, floor_y, body_top, belt_y, roof_hw, roof_y
    (0.00,  0.38, 0.08, 0.48, 0.48, 0.30, 0.48),  # rear end
    (0.05,  0.46, 0.07, 0.50, 0.48, 0.34, 0.50),  # rear bumper
    (0.10,  0.52, 0.06, 0.52, 0.46, 0.38, 0.52),  # rear fender start
    (0.18,  0.54, 0.06, 0.54, 0.44, 0.42, 0.54),  # rear quarter
    (0.25,  0.55, 0.06, 0.55, 0.42, 0.44, 0.72),  # C-pillar base
    (0.30,  0.55, 0.06, 0.55, 0.40, 0.44, 0.82),  # rear window
    (0.38,  0.54, 0.06, 0.54, 0.38, 0.43, 0.88),  # roof peak
    (0.45,  0.53, 0.06, 0.53, 0.38, 0.42, 0.90),  # roof middle
    (0.52,  0.53, 0.06, 0.53, 0.38, 0.42, 0.88),  # roof front
    (0.58,  0.54, 0.06, 0.54, 0.40, 0.40, 0.80),  # A-pillar
    (0.65,  0.55, 0.06, 0.55, 0.44, 0.36, 0.65),  # windshield base
    (0.72,  0.54, 0.06, 0.52, 0.52, 0.30, 0.52),  # hood rear
    (0.80,  0.52, 0.07, 0.48, 0.48, 0.28, 0.48),  # hood middle
    (0.88,  0.48, 0.08, 0.44, 0.44, 0.26, 0.44),  # hood front
    (0.93,  0.44, 0.09, 0.40, 0.40, 0.24, 0.40),  # front bumper
    (0.97,  0.40, 0.10, 0.36, 0.36, 0.22, 0.36),  # nose
    (1.00,  0.32, 0.12, 0.32, 0.32, 0.18, 0.32),  # front tip
]

def get_profile_at(z_norm):
    """Interpolate profile at arbitrary z_norm using Catmull-Rom."""
    # Find surrounding profiles
    for i in range(len(raw_profiles) - 1):
        if raw_profiles[i][0] <= z_norm <= raw_profiles[i+1][0]:
            t = (z_norm - raw_profiles[i][0]) / (raw_profiles[i+1][0] - raw_profiles[i][0])
            # Get 4 control points for Catmull-Rom
            i0 = max(0, i - 1)
            i1 = i
            i2 = i + 1
            i3 = min(len(raw_profiles) - 1, i + 2)
            result = [z_norm]
            for ch in range(1, 7):  # interpolate each channel
                val = catmull_rom(
                    raw_profiles[i0][ch], raw_profiles[i1][ch],
                    raw_profiles[i2][ch], raw_profiles[i3][ch], t)
                result.append(val)
            return result
    return raw_profiles[-1]


def body_cross_section(profile, n_segments):
    """Generate points around the body cross-section at a given profile.
    Returns list of (x, y) points going clockwise from bottom-left."""
    _, hw, floor_y, body_top, belt_y, roof_hw, roof_y = profile

    points = []
    # We trace: bottom-left -> left side up -> roof left -> roof right -> right side down -> bottom-right -> bottom
    # Bottom (flat)
    n_bottom = n_segments // 4
    for i in range(n_bottom + 1):
        t = i / n_bottom
        x = lerp(-hw, hw, t)
        points.append((x, floor_y))

    # Right side going up (curved)
    n_side = n_segments // 3
    for i in range(1, n_side + 1):
        t = i / n_side
        y = lerp(floor_y, body_top, smoothstep(t))
        # Slight outward bulge for fender shape
        bulge = math.sin(t * math.pi) * 0.02
        points.append((hw + bulge, y))

    # Right shoulder to roof
    n_shoulder = n_segments // 6
    for i in range(1, n_shoulder + 1):
        t = i / n_shoulder
        x = lerp(hw, roof_hw, smoothstep(t))
        y = lerp(body_top, roof_y, smoothstep(t))
        points.append((x, y))

    # Roof (slight crown)
    n_roof = n_segments // 4
    for i in range(1, n_roof):
        t = i / n_roof
        x = lerp(roof_hw, -roof_hw, t)
        crown = math.sin(t * math.pi) * 0.015
        points.append((x, roof_y + crown))

    # Left shoulder from roof
    for i in range(1, n_shoulder + 1):
        t = i / n_shoulder
        x = lerp(-roof_hw, -hw, smoothstep(t))
        y = lerp(roof_y, body_top, smoothstep(t))
        points.append((x, y))

    # Left side going down
    for i in range(1, n_side + 1):
        t = i / n_side
        y = lerp(body_top, floor_y, smoothstep(t))
        bulge = math.sin((1-t) * math.pi) * 0.02
        points.append((-hw - bulge, y))

    return points


def generate_body():
    """Generate the car body mesh by lofting cross-sections."""
    obj = OBJWriter()

    n_stations = 60  # lengthwise resolution
    n_circ = 32      # circumferential resolution per section

    # Generate all cross-sections
    sections = []
    for i in range(n_stations + 1):
        z_norm = i / n_stations
        profile = get_profile_at(z_norm)
        z_world = lerp(-CAR_HALF_LENGTH, CAR_HALF_LENGTH, z_norm)
        pts = body_cross_section(profile, n_circ)
        # Convert to 3D
        section_3d = [(p[0], p[1], z_world) for p in pts]
        sections.append(section_3d)

    # Loft: connect adjacent sections with quads
    for i in range(len(sections) - 1):
        s0 = sections[i]
        s1 = sections[i + 1]
        n = min(len(s0), len(s1))
        for j in range(n - 1):
            p0 = s0[j]
            p1 = s0[j + 1]
            p2 = s1[j + 1]
            p3 = s1[j]
            obj.add_quad_smooth(p0, p1, p2, p3)

    # Cap the rear
    rear = sections[0]
    cx = sum(p[0] for p in rear) / len(rear)
    cy = sum(p[1] for p in rear) / len(rear)
    cz = rear[0][2]
    for j in range(len(rear) - 1):
        obj.add_tri((cx, cy, cz), rear[j+1], rear[j])

    # Cap the front
    front = sections[-1]
    cx = sum(p[0] for p in front) / len(front)
    cy = sum(p[1] for p in front) / len(front)
    cz = front[0][2]
    for j in range(len(front) - 1):
        obj.add_tri((cx, cy, cz), front[j], front[j+1])

    # Undercarriage (flat bottom)
    hw = 0.50
    obj.add_quad_smooth(
        (-hw, 0.06, -CAR_HALF_LENGTH),
        ( hw, 0.06, -CAR_HALF_LENGTH),
        ( hw, 0.06,  CAR_HALF_LENGTH),
        (-hw, 0.06,  CAR_HALF_LENGTH))

    obj.save("assets/car_body.obj")


def generate_glass():
    """Windshield, rear window, side windows."""
    obj = OBJWriter()

    # Windshield (angled quad)
    obj.add_quad_smooth(
        (-0.42, 0.54, 0.30),
        ( 0.42, 0.54, 0.30),
        ( 0.36, 0.80, 0.05),
        (-0.36, 0.80, 0.05))

    # Rear window
    obj.add_quad_smooth(
        ( 0.40, 0.54, -0.40),
        (-0.40, 0.54, -0.40),
        (-0.36, 0.78, -0.20),
        ( 0.36, 0.78, -0.20))

    # Left side window
    obj.add_quad_smooth(
        (-0.54, 0.40, -0.30),
        (-0.54, 0.40,  0.25),
        (-0.44, 0.82,  0.02),
        (-0.42, 0.76, -0.22))

    # Right side window
    obj.add_quad_smooth(
        ( 0.54, 0.40,  0.25),
        ( 0.54, 0.40, -0.30),
        ( 0.42, 0.76, -0.22),
        ( 0.44, 0.82,  0.02))

    obj.save("assets/car_glass.obj")


def generate_wheel(cx, cy, cz, radius, width, left_side, obj):
    """Generate a detailed wheel with tire treads and spoked rim."""
    segments = 24
    tread_segments = 6

    # Tire outer surface (torus-like)
    for i in range(segments):
        a0 = 2 * math.pi * i / segments
        a1 = 2 * math.pi * (i + 1) / segments

        for j in range(tread_segments):
            t0 = j / tread_segments
            t1 = (j + 1) / tread_segments
            # Angle around the cross-section of the tire
            b0 = -math.pi/2 + math.pi * t0
            b1 = -math.pi/2 + math.pi * t1

            tread_r = radius * 0.15  # cross-section radius of tire

            def tire_point(a, b):
                r = radius - tread_r + tread_r * math.cos(b)
                w = width/2 * (2*t0 - 1) if b == b0 else width/2 * (2*t1 - 1)
                # Actually compute properly
                local_r = radius - tread_r + tread_r * math.cos(b)
                local_w = tread_r * math.sin(b)
                x = cx + local_w
                y = cy + local_r * math.sin(a)
                z = cz + local_r * math.cos(a)
                return (x, y, z)

            # Simplified: just make a nice cylinder with rounded edges
            pass

        # Tire sidewall (outer ring)
        cos0, sin0 = math.cos(a0), math.sin(a0)
        cos1, sin1 = math.cos(a1), math.sin(a1)

        # Outer tread surface
        hw = width / 2
        p0 = (cx - hw, cy + radius * sin0, cz + radius * cos0)
        p1 = (cx + hw, cy + radius * sin0, cz + radius * cos0)
        p2 = (cx + hw, cy + radius * sin1, cz + radius * cos1)
        p3 = (cx - hw, cy + radius * sin1, cz + radius * cos1)
        obj.add_quad_smooth(p0, p1, p2, p3)

        # Inner rim surface
        rim_r = radius * 0.55
        p0 = (cx - hw*0.6, cy + rim_r * sin0, cz + rim_r * cos0)
        p1 = (cx + hw*0.6, cy + rim_r * sin0, cz + rim_r * cos0)
        p2 = (cx + hw*0.6, cy + rim_r * sin1, cz + rim_r * cos1)
        p3 = (cx - hw*0.6, cy + rim_r * sin1, cz + rim_r * cos1)
        obj.add_quad_smooth(p0, p1, p2, p3)

        # Sidewall connecting tread to rim (left)
        p0 = (cx - hw, cy + radius * sin0, cz + radius * cos0)
        p1 = (cx - hw, cy + radius * sin1, cz + radius * cos1)
        p2 = (cx - hw*0.6, cy + rim_r * sin1, cz + rim_r * cos1)
        p3 = (cx - hw*0.6, cy + rim_r * sin0, cz + rim_r * cos0)
        obj.add_quad_smooth(p0, p1, p2, p3)

        # Sidewall (right)
        p0 = (cx + hw, cy + radius * sin1, cz + radius * cos1)
        p1 = (cx + hw, cy + radius * sin0, cz + radius * cos0)
        p2 = (cx + hw*0.6, cy + rim_r * sin0, cz + rim_r * cos0)
        p3 = (cx + hw*0.6, cy + rim_r * sin1, cz + rim_r * cos1)
        obj.add_quad_smooth(p0, p1, p2, p3)

    # Spokes (5 spokes)
    n_spokes = 5
    spoke_w = 0.02
    for s in range(n_spokes):
        angle = 2 * math.pi * s / n_spokes
        cos_a, sin_a = math.cos(angle), math.sin(angle)

        inner_r = radius * 0.1
        outer_r = rim_r * 0.95

        # Spoke as a thin quad
        perp_y = -cos_a * spoke_w
        perp_z = sin_a * spoke_w

        for side_x in [-hw*0.3, hw*0.3]:
            p0 = (side_x, cy + inner_r * sin_a + perp_y, cz + inner_r * cos_a + perp_z)
            p1 = (side_x, cy + outer_r * sin_a + perp_y, cz + outer_r * cos_a + perp_z)
            p2 = (side_x, cy + outer_r * sin_a - perp_y, cz + outer_r * cos_a - perp_z)
            p3 = (side_x, cy + inner_r * sin_a - perp_y, cz + inner_r * cos_a - perp_z)
            obj.add_quad_smooth(p0, p1, p2, p3)


def generate_wheels():
    """Generate all 4 wheels."""
    obj = OBJWriter()
    wheel_r = 0.18
    wheel_w = 0.12
    wheel_y = wheel_r

    positions = [
        (-0.56, wheel_y,  0.65, True),   # front-left
        ( 0.56, wheel_y,  0.65, False),  # front-right
        (-0.56, wheel_y, -0.65, True),   # rear-left
        ( 0.56, wheel_y, -0.65, False),  # rear-right
    ]

    for cx, cy, cz, left in positions:
        generate_wheel(cx, cy, cz, wheel_r, wheel_w, left, obj)

    obj.save("assets/car_wheels.obj")


def generate_lights():
    """Headlights and taillights geometry."""
    obj = OBJWriter()

    # Headlights (front, two units)
    for side in [-1, 1]:
        cx = side * 0.35
        # Rounded headlight shape (low-poly circle)
        segments = 12
        r = 0.08
        cz = CAR_HALF_LENGTH + 0.02
        cy_center = 0.38
        for i in range(segments):
            a0 = 2 * math.pi * i / segments
            a1 = 2 * math.pi * (i + 1) / segments
            p0 = (cx, cy_center, cz)
            p1 = (cx + r * math.cos(a0), cy_center + r * math.sin(a0), cz)
            p2 = (cx + r * math.cos(a1), cy_center + r * math.sin(a1), cz)
            obj.add_tri(p0, p1, p2)

    # Taillights (rear, wider)
    for side in [-1, 1]:
        x0 = side * 0.20
        x1 = side * 0.48
        if side < 0:
            x0, x1 = x1, x0
        cz = -CAR_HALF_LENGTH - 0.02
        obj.add_quad_smooth(
            (x0, 0.35, cz), (x1, 0.35, cz),
            (x1, 0.48, cz), (x0, 0.48, cz))

    # DRL strips (thin light bars under headlights)
    for side in [-1, 1]:
        x0 = side * 0.22
        x1 = side * 0.48
        if side < 0:
            x0, x1 = x1, x0
        cz = CAR_HALF_LENGTH + 0.02
        obj.add_quad_smooth(
            (x0, 0.28, cz), (x1, 0.28, cz),
            (x1, 0.31, cz), (x0, 0.31, cz))

    obj.save("assets/car_lights.obj")


def generate_details():
    """Spoiler, mirrors, grille, exhaust tips, etc."""
    obj = OBJWriter()

    # Rear spoiler
    spoiler_w = 0.42
    spoiler_z0 = -0.55
    spoiler_z1 = -0.45
    spoiler_y = 0.58
    spoiler_thick = 0.02

    # Spoiler blade
    obj.add_quad_smooth(
        (-spoiler_w, spoiler_y, spoiler_z0),
        ( spoiler_w, spoiler_y, spoiler_z0),
        ( spoiler_w, spoiler_y, spoiler_z1),
        (-spoiler_w, spoiler_y, spoiler_z1))
    obj.add_quad_smooth(
        (-spoiler_w, spoiler_y - spoiler_thick, spoiler_z1),
        ( spoiler_w, spoiler_y - spoiler_thick, spoiler_z1),
        ( spoiler_w, spoiler_y - spoiler_thick, spoiler_z0),
        (-spoiler_w, spoiler_y - spoiler_thick, spoiler_z0))
    # Spoiler endplates
    for side in [-1, 1]:
        x = side * spoiler_w
        obj.add_quad_smooth(
            (x, spoiler_y - spoiler_thick, spoiler_z0),
            (x, spoiler_y, spoiler_z0),
            (x, spoiler_y, spoiler_z1),
            (x, spoiler_y - spoiler_thick, spoiler_z1))

    # Spoiler supports
    for side in [-1, 1]:
        x = side * 0.30
        obj.add_quad_smooth(
            (x - 0.015, 0.50, -0.52),
            (x + 0.015, 0.50, -0.52),
            (x + 0.015, spoiler_y - spoiler_thick, -0.52),
            (x - 0.015, spoiler_y - spoiler_thick, -0.52))

    # Side mirrors
    for side in [-1, 1]:
        mx = side * 0.56
        my = 0.52
        mz = 0.15
        ms = 0.04
        # Mirror housing (small box)
        obj.add_quad_smooth(
            (mx - ms, my - ms, mz - ms*2),
            (mx + ms, my - ms, mz - ms*2),
            (mx + ms, my + ms, mz - ms*2),
            (mx - ms, my + ms, mz - ms*2))
        obj.add_quad_smooth(
            (mx + ms, my - ms, mz + ms*2),
            (mx - ms, my - ms, mz + ms*2),
            (mx - ms, my + ms, mz + ms*2),
            (mx + ms, my + ms, mz + ms*2))
        # Mirror stalk
        obj.add_quad_smooth(
            (mx - 0.01, my - 0.01, mz - ms*2),
            (mx + 0.01, my - 0.01, mz - ms*2),
            (mx + 0.01, my - 0.01, mz + ms*2),
            (mx - 0.01, my - 0.01, mz + ms*2))

    # Front grille
    grille_hw = 0.28
    grille_y0 = 0.12
    grille_y1 = 0.32
    grille_z = CAR_HALF_LENGTH + 0.01
    # Horizontal slats
    n_slats = 5
    for i in range(n_slats):
        t = i / (n_slats - 1)
        y = lerp(grille_y0, grille_y1, t)
        obj.add_quad_smooth(
            (-grille_hw, y, grille_z),
            ( grille_hw, y, grille_z),
            ( grille_hw, y + 0.015, grille_z),
            (-grille_hw, y + 0.015, grille_z))

    # Exhaust tips (two circles at rear)
    for side in [-1, 1]:
        cx = side * 0.25
        cy = 0.12
        cz = -CAR_HALF_LENGTH - 0.03
        r = 0.035
        segments = 10
        for i in range(segments):
            a0 = 2 * math.pi * i / segments
            a1 = 2 * math.pi * (i + 1) / segments
            # Outer ring
            obj.add_quad_smooth(
                (cx + r*math.cos(a0), cy + r*math.sin(a0), cz),
                (cx + r*math.cos(a1), cy + r*math.sin(a1), cz),
                (cx + r*math.cos(a1), cy + r*math.sin(a1), cz - 0.04),
                (cx + r*math.cos(a0), cy + r*math.sin(a0), cz - 0.04))

    obj.save("assets/car_details.obj")


if __name__ == "__main__":
    print("Generating car model...")
    generate_body()
    generate_glass()
    generate_wheels()
    generate_lights()
    generate_details()
    print("Done! Models saved to assets/")
