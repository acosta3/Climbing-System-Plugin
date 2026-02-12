# 🧗‍♂️ Climbing System Plugin

A modular **Climbing System plugin** for **Unreal Engine 5.6**, built in **C++** and designed for **third-person games**.  
Supports ledge grabbing, climbing up and down, and traversal-ready extensions.

<p align="center">
  <a href="https://www.youtube.com/watch?v=bDSDSq6FjFE">
    <img src="https://img.youtube.com/vi/bDSDSq6FjFE/0.jpg" width="800" />
  </a>
</p>

---
#  Advanced Climbing System

##  What Makes This Special

###  Adaptive Motion on Uneven Geometry
The climbing system uses **Inverse Kinematics (IK)** and **Motion Warping** to create smooth, natural-looking traversal **even on jagged, irregular surfaces**.

---

## The Challenge

Real game environments have uneven ledges, angled surfaces, and varying heights. Traditional animation systems look robotic and break immersion when characters snap to fixed positions. 

**The hardest part was accurately detecting climbable surfaces and determining proper character placement/orientation on irregular geometry without performance issues or visual artifacts.**

---

## ✅ The Solution

### Core Systems

- **Capsule Trace Detection**  
  Uses the character's capsule for initial surface detection instead of single-point raycasts

- **Surface-Aligned Orientation**  
  Character rotation uses the **surface's normal vector** (obtained via capsule trace) **rather than the character's own orientation**, allowing natural snapping to walls at any angle

- **Multi-Actor Averaging**  
  Orientation calculated by averaging normals from **multiple hit actors** (limited to a fixed-size array for performance)

- **Location Validation**  
  Since averaging multiple points can produce "fake" locations that don't actually exist on geometry, the system **line traces back to the surface** to verify and correct placement

- **Smooth Interpolation**  
  Uses **FInterp (linear interpolation)** for snapping behavior, eliminating jarring position changes

- **Custom Physics Mode**  
  Leveraged and reworked Unreal's **Flying movement mode** to handle wall-attached traversal physics

- **IK-Driven Hand/Foot Placement**  
  Character limbs dynamically adjust to surface geometry in real-time

- *Motion Warping**  
  Animations adapt their root motion to match actual ledge positions

---



---

## 📦 Plugin Name
**ClimbSystem**

---

## ⚙️ Requirements
- **Unreal Engine:** 5.6.x  
- **Project Type:** C++ ThirdPerson project  
  - Must contain a `Source/` folder  
  - Blueprint-only projects are **not supported**
- **IDE:** Visual Studio  
  - Installed with **Game development with C++** workload

---

## 🚀 Getting Started

1. Create or open a **C++ ThirdPerson** Unreal Engine project.
2. Copy the plugin into your project's `Plugins/` directory.
3. Regenerate project files and rebuild the project.
4. Enable the plugin in Unreal Editor if needed.
5. Press **Play** and start climbing (default keybind: **F**).

---

## 📘 Documentation

**[How to Set Up the ClimbSystem Plugin (PDF)](https://github.com/acosta3/Climbing-System-Plugin/blob/main/ClimbSystem/Doc/How%20To%20Setup%20Plugin.pdf)**  
- 📖 Illustrated, step-by-step setup guide with screenshots showing how to integrate the plugin into a C++ Unreal Engine 5.6 project.
  
- 🎥 Optional setup walkthrough video (Windows example, step-by-step):  
  [How To Set Up The Plugin](https://youtube.com/watch?v=rnQOWEQ2M0Y&feature=youtu.be)

---

## 🎥 Demo

Click the demo image above to watch the climbing system in action on YouTube.

---

## 🧩 Features

### Core Mechanics
- ✅ **Ledge Detection** - Capsule sweeps + multi-point line traces with surface normal validation
- ✅ **Climb Up/Down** - Smooth vertical traversal with IK hand placement
- ✅ **Hang State** - Stable ledge grip with physics-based holding
- ✅ **Hop Up/Down** - Quick ledge transitions for small obstacles


### Extensibility
Designed as a foundation for advanced traversal systems:
- Wall climbing
- Vaulting/mantling
- Corner turning
- Moving platform support

---

## 💡 Technical Deep Dive

### IK + Motion Warping Pipeline
```cpp
1. Surface Detection
   └─> Capsule sweep finds potential ledges
   └─> Line traces validate surface normals
   └─> Check walkable angle thresholds

2. Motion Warping Setup
   └─> Calculate target transform from ledge position
   └─> Warp animation root motion to match
   └─> Blend in over configurable time

3. IK Application (During Climb)
   └─> Trace from hand bones to find actual surface points
   └─> Apply IK to adjust hand/foot positions
   └─> Smooth blend to prevent jitter on jagged geometry
   └─> Update every frame for dynamic surfaces
```

### Why This Matters
- **Believable Animation** - Characters feel grounded in the world
- **Level Design Freedom** - Artists don't need perfectly smooth ledges
- **Gameplay Variety** - Works on rocks, buildings, ruins - any climbable surface
- **Performance** - IK calculations are localized to active climbing states

---


---

---

## 🤝 Contributing

Contributions, issues, and feature requests are welcome!

---



[![YouTube](https://img.shields.io/badge/YouTube-Demo-red?style=for-the-badge&logo=youtube&logoColor=white)](https://www.youtube.com/watch?v=bDSDSq6FjFE)
[![GitHub](https://img.shields.io/badge/GitHub-Star-181717?style=for-the-badge&logo=github&logoColor=white)](https://github.com/acosta3/Climbing-System-Plugin)

</div>
