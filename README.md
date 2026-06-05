# Zombie Survivor AI

![Unreal Engine](https://img.shields.io/badge/Unreal_Engine-0e1128?style=for-the-badge&logo=unrealengine&logoColor=white)
![C++](https://img.shields.io/badge/C++-Scripting-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)
![AI](https://img.shields.io/badge/AI-Behavior_Trees-8A2BE2?style=for-the-badge)

**Author:** Ana Paoliello de Lucena Carvalho | **Course:** Algorithms 2 (DAE - Howest)

> **Project Mission:** Develop an autonomous survival agent in a zombie apocalypse environment using Unreal Engine. The agent relies strictly on simulated senses, dynamic memory, and prioritised decision-making to scavenge for loot, manage stats, and utilise algorithmic steering to fight or flee, all without relying on hardcoded waypoints or Tick-based logic.

---

## ⚖️ Emergent Gameplay
Depending on its real-time inventory and surrounding threat analysis, the agent transitions between a **scavenger** and a **survivor**, creating organic, unpredictable gameplay scenarios.

---

## 🧠 Core Systems & Algorithms

### Movement: Blended Steering
Bypassing Unreal Engine's standard `MoveTo` navigation, the AI utilises a custom, mathematically **Blended Steering** system. It calculates and combines up to 6 behavioural forces:

* **Path-Following:** Calculates waypoint routes to items and doorways.
* **Wander:** Walks around the map.
* **Flee:** Pushes the agent away from threats.
* **Obstacle Avoidance:** Uses 3 "whiskers" (raycasts) to detect walls and push the agent away from collisions.
* **House Containment:** Prevents the agent from bumping into walls while actively looting inside tight, narrow spaces.
* **Stuck Detection Escape:** An 8-ray burst that fires if the AI becomes trapped, finding the most open path to forcibly extract itself.

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
The AI treats its limited backpack slots as a vital resource, requiring strict resource management logic:
* **Smart Looting:** Evaluates items before picking them up. It destroys "Garbage" to clean the map, and only pockets Food and Medkits if empty slots are available, and only picks up Weapons if one is not already in the backpack.
* **Item Usage & Auto-Discard:** If stats drop, the AI consumes items until it is maxed out. Empty items or depleted weapons are instantly discarded to free up space.
* **Photographic Memory:** If the inventory is full but a valuable item is spotted, the agent logs it into a KnownItems array. If stats drop to critical levels later, it retrieves this memory and navigates back to the item to survive.
* **Amnesia Prevention:** Once a house is scouted and exited, it is added to a VisitedHouses array.

---

## ⚙️ Implementation: The Behaviour Tree
The core of this project is a massive, strictly prioritised Behaviour Tree. It evaluates branches from Left (Highest Priority) to Right (Lowest Priority), utilising Observer Aborts to instantly cancel actions when emergencies occur.

### The Logic Hierarchy
1. **PURGE EVASION:** Instantly flee if a purge zone is detected.
2. **TACTICAL RETREAT:** If unarmed, sprint away. (Also handles desperate Last-Stand healing/eating if stats are critical).
3. **COMBAT:** If armed and facing a normal threat, rotate, aim, and shoot.
4. **CRITICAL NEEDS:** If Health/Stamina < 7, consume Medkits/Food from inventory.
5. **Emergency Heal / Energy:** If Health < 30 or Stamina < 30, consume Medkits/Food from inventory, or run to pick them up from the floor.
6. **SCAVENGE (Loot Floor):** Move to and pocket nearby needed items.
7. **SCOUT HOUSE:** Walk inside unseen houses to establish line-of-sight for loot.
8. **EXIT HOUSE:** Navigate cleanly out of the doorway.
9. **EXPLORE:** Calculate coverage-biased wander math.

---

## 🔚 Conclusion & Takeaways
Building an autonomous agent from scratch is less about telling it what to do perfectly, and more about writing robust fallback logic for when it gets confused. Here are the biggest lessons learned:
1. **The "Disconnected Brain" Bug:** I spent hours debugging complex Behaviour Tree abort loops and memory amnesia, only to discover that an Unreal Engine Data Asset had disconnected my `StudentPerceptor` component from my Pawn! The code was technically working; the agent just didn't have its brain plugged into its body. It taught me to always verify engine-level component references before rewriting C++ logic.
2. **Behaviour Tree Abort Loops:** Setting and clearing Blackboard variables must be handled flawlessly. If a "Scout House" state finishes without clearing its Blackboard key, the tree would instantly abort lower-priority tasks (Exiting) to endlessly restart the Scout phase.
3. **Algorithmic Recovery:** AI will inevitably get stuck. Building the 8-ray Stuck Detection burst taught me that a good AI needs built-in panic responses to forcibly extract itself from edge cases.

---

*Developed by Ana Paoliello de Lucena Carvalho for the Algorithms 2 course at DAE (Howest)*
