# Zombie Survivor AI - Autonomous Behaviour System
![Unreal Engine](https://img.shields.io/badge/Unreal_Engine-TODO:version-0e1128?style=for-the-badge&logo=unrealengine&logoColor=white)
![C++](https://img.shields.io/badge/C++-Scripting-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)
![AI](https://img.shields.io/badge/AI-Behavior_Trees-8A2BE2?style=for-the-badge)

**Author:** Ana Paoliello de Lucena Carvalho | **Course:** Algorithms 2  
> **Project Mission:** Develop an autonomous survival agent in a zombie apocalypse environment using Unreal Engine. The agent must rely strictly on simulated senses, dynamic memory, and prioritised decision-making to scavenge for loot, manage stats, and utilise algorithmic steering behaviours to fight or flee, without relying on hardcoded waypoints or Tick-based logic.

<img width="1000" alt="Zombie AI Gameplay Demo" src="TODO add gift" />

---

## ⚖️ The Result & Emergent Gameplay
The agent transitions between a scavenger and a survivor depending on its inventory and the surrounding threats.

---

## 🧠 Core Systems & Algorithms

### Movement - Blended Steering
Instead of relying on Unreal Engine's standard `MoveTo` navigation, the AI uses a custom, mathematically Blended Steering system that calculates and combines up to 6 distinct behavioural forces every frame:
* **NavMesh Path-Following:** Calculates waypoint routes to items and doorways.
* **Coverage-Biased Wander:** Sweeps the map fluidly rather than picking rigid random points.
* **Flee:** Pushes the agent directly away from immediate threats.
* **Obstacle Avoidance:** Uses 3 dynamic "whiskers" (raycasts) to detect walls and push the agent away from collisions.
* **House Containment:** Prevents the agent from bumping into walls while actively looting inside narrow rooms.
* **Stuck Detection Escape:** An 8-ray burst that fires if the AI is trapped, finding the most open path to forcibly extract itself.

```cpp
if (bIsInside && !DoorwayLoc.IsNearlyZero())
{
    FVector ToDoor = (DoorwayLoc - Pawn->GetActorLocation()).GetSafeNormal2D();
    DesiredDirection = (FleeForce * 0.5f) + (ToDoor * 0.5f);
}
else
{
    DesiredDirection = (FleeForce * 0.85f) + (WanderForce * 0.15f);
}
```

### Inventory & Memory Management
The AI treats its limited backpack slots as a vital resource for survival:
* **Smart Looting:** Evaluates items before picking them up. It destroys "Garbage" to clean the map, and only pockets Weapons, Food, and Medkits if there are empty slots.
* **Item Usage & Auto-Discard:** If stats drop, the AI continuously consumes items until stats are maxed. Once an item or weapon is empty, it is instantly discarded to free up space.
* **Photographic Memory:** If the inventory is full but a valuable item is spotted, the agent logs it into a `KnownItems` array. Later, if stats drop to critical levels, it retrieves the memory and navigates back to survive.
* **Amnesia Prevention:** Once a house is scouted and exited, it is added to a `VisitedHouses` array. The AI ignores this house for the rest of the game, forcing it to explore new territory.

---

## ⚙️ Implementation: The Behaviour Tree
The core of this project is a massive, strictly prioritised Behaviour Tree. It evaluates branches from Left (Highest Priority) to Right (Lowest Priority), utilising Observer Aborts to instantly cancel actions when emergencies occur.

### The Logic Hierarchy
1. **PURGE EVASION:** Instantly flee if a death zone is detected.
2. **TACTICAL RETREAT:** If unarmed or facing a Heavy Zombie, sprint away. (Also handles desperate Last-Stand healing/eating if stats are critical).
3. **COMBAT:** If armed and facing a normal threat, rotate, aim, and shoot.
4. **CRITICAL NEEDS:** If Health/Stamina < 7, consume Medkits/Food from inventory.
5. **Emergency Heal / Energy:** If Health < 30 or Stamina < 30, consume Medkits/Food from inventory, or run to pick them up from the floor.
6. **SCAVENGE (Loot Floor):** Move to and pocket nearby needed items.
7. **SCOUT HOUSE:** Walk inside unseen houses to establish line-of-sight for loot.
8. **EXIT HOUSE:** Navigate cleanly out of the doorway.
9. **EXPLORE:** Calculate coverage-biased wander math.

---

## 🔚 Conclusion & Takeaways
The greatest lessons learned during this project involved dealing with Logic Traps and the realities of game architecture:
1. **The "Disconnected Brain" Bug:** I spent hours debugging complex Behaviour Tree abort loops and memory amnesia, only to discover that an Unreal Engine Data Asset had disconnected my `StudentPerceptor` component from my Pawn due to a file rename! The code was mathematically perfect; the agent just didn't have its brain plugged into its body. It taught me to always verify engine-level component references before rewriting complex C++ logic.
2. **Behaviour Tree Abort Loops:** I learned that setting and clearing Blackboard variables must be handled incredibly carefully. If a "Scout House" state finishes without clearing its Blackboard key, the tree will instantly abort lower-priority tasks (like Exiting) to endlessly restart the Scout phase.
3. **Algorithmic Recovery:** Building this AI taught me that creating autonomous agents is less about telling them what to do perfectly, and more about writing robust fallback logic (like my 8-ray Stuck Detection burst) to help them recover when they inevitably get confused.

---
*Developed by Ana Paoliello de Lucena Carvalho for the Algorithms 2 course at DAE (Howest)*
