# Zombie Survivor AI - Autonomous Behaviour System
![Unreal Engine](https://img.shields.io/badge/Unreal_Engine-TODO:version-0e1128?style=for-the-badge&logo=unrealengine&logoColor=white)
![C++](https://img.shields.io/badge/C++-Scripting-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)
![AI](https://img.shields.io/badge/AI-Behavior_Trees-8A2BE2?style=for-the-badge)

**Author:** Ana Paoliello de Lucena Carvalho | **Course:** Algorithms 2  
> **Project Mission:** Develop an autonomous survival agent in a zombie apocalypse environment using Unreal Engine. The agent must rely strictly on simulated senses, dynamic memory and prioritised decision-making to scavenge for loot, manage stats, and utilise algorithmic steering behaviours to fight or flee, without relying on hardcoded waypoints or Tick-based logic.

<img width="1000" alt="Zombie AI Gameplay Demo" src="TODO add gift" />

---

## ⚖️ The Result & Emergent Gameplay
Building this AI proved that strict, prioritised logic creates dynamic and emergent gameplay. The agent transitions between scavenger and survivor depending on its inventory and the threats around it.

### Scenario A - Unarmed Scavenger
| Action | Perception Trigger | Blackboard Memory | Outcome |
| :--- | :--- | :--- | :--- |
| **House Scouting** | Agent sees a House (`AHouse`) | `NearestHouse` is Set | Aborts wandering, moves inside to establish line of sight on hidden loot |
| **Smart Looting** | Agent sees items inside | `NearestItem` is Set | Ignores garbage, prioritises Weapons and Medkits, and hides items upon pickup |
| **Tactical Retreat** | Fast Zombie approaches, but the agent has no gun | `NearestZombie` = Set, `HasWeapon` = False | Agent realises it cannot win, calculates an **Evade Point**, and sprints away to survive |

### Scenario B - Armed Survivor
| Action | Perception Trigger | Blackboard Memory | Outcome |
| :--- | :--- | :--- | :--- |
| **Self Defence** | Normal Zombie approaches | `NearestZombie` = Set, `HasWeapon` = True | Agent stops, rotates to face the threat, and fires until the zombie dies |
| **Ammo Management** | Gun runs out of ammo | `Item->GetValue() == 0` | Agent throws the empty gun away, resetting `HasWeapon` to False to avoid staring at enemies with an empty clip |
| **Threat Assessment** | **Heavy Zombie** approaches | `IsHeavyZombie` = True | Agent recognises the high threat, overrides the Fight branch, and chooses to flee despite being armed |

---

## 🧬 Research and Algorithms

### Steering Behaviours
Instead of relying on Unreal Engine's `MoveTo` navigation, the AI uses custom math from **Algorithms 2** to calculate dynamic, safe coordinates.

<details>
<summary><b>📖 Read More & Code (Wander & Evade)</b></summary>

* **Wander Algorithm:** To create natural exploration, the AI projects a circle in front of its forward vector. It calculates a random angle (`Sin`/`Cos`) on that circle's circumference to pick a fluid, meandering target point. This sweeps the agent's vision cone across the map to find houses and items.

* **Evade Algorithm:** When fleeing, the AI calculates a normalised directional vector pointing *from* the zombie *to* the player, multiplies it by an escape distance, and sets a dynamic safe zone to sprint toward.

```cpp
// TODO - add code here
```
</details>

### AI Perception
The `StudentPerceptor` acts as the agent's eyes, utilising `AIPerception` to filter the world. 

<details>
<summary><b>📖 Read More & Code</b></summary>
  
The Perceptor is designed to remember only things that aid in survival. It completely ignores useless actors (like Garbage) and categorises threats and assets dynamically.

```cpp
// TODO - add code here
```
</details>

---

## ⚙️ Implementation

The core of this project is a massive, strictly prioritised **Behaviour Tree**. It evaluates branches from Left (Highest Priority) to Right (Lowest Priority), utilising **Observer Aborts** to instantly cancel actions when emergencies happen.

### The Logic Hierarchy
1. **Purge Avoidance:** Instantly calculate Evade if a death zone is detected.
2. **Emergency Flee:** If Health < 30 and chased, hide in a House or run.
3. **Fight:** If armed, face the zombie and shoot (Wait 1s).
4. **Tactical Retreat:** If unarmed (or facing a Heavy Zombie), Evade!
5. **Emergency Heal / Energy:** If Health < 30 or Stamina < 30, consume Medkits/Food from inventory, or run to pick them up from the floor.
6. **Active Scavenge:** Move to and pocket nearby items.
7. **Scout House:** If a house is seen, walk inside to establish line-of-sight for loot.
8. **Explore:** Calculate Wander math.

<details>
<summary><b>📖 Read More: Custom Tasks & Memory Wipes</b></summary>
  
A major hurdle in AI development is the "Memory Loop" (e.g., trying to pick up an item that is already in the inventory, or freezing on a wall).

* **Ghost Item Fix:** When an item is looted, it is set to `HiddenInGame` and collision is disabled so the Perceptor stops "seeing" it. (LATER SEE THIS AGAIN)
* **Dynamic Memory Clearing:** Created a custom C++ Task (`UBTT_ClearMemory`) with a `FBlackboardKeySelector`. This allows the Behaviour Tree to wipe specific memories (like `NearestHouse`) the moment the agent steps through the door, forcing the AI to evaluate the *inside* of the house rather than staring at the wall indefinitely.

```cpp
// UBTT_ClearMemory.cpp
EBTNodeResult::Type UUBTT_ClearMemory::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    UBlackboardComponent* BBComp = OwnerComp.GetBlackboardComponent();
    // Dynamically clears whichever key is selected in the Unreal BT Editor dropdown!
    BBComp->ClearValue(KeyToClear.SelectedKeyName);
    return EBTNodeResult::Succeeded;
}
```
</details>

---

## ✅ Exam Requirements Checklist

- [x] **Clear Decision Structure:** Left-to-right Behavior Tree logic.
- [x] **No Tick Spaghetti:** Decisions are event-driven via AI Perception and BT Services; no massive `if/else` chains in `Tick()`.
- [x] **AIPerception Only:** Zero hardcoded map locations. The agent dynamically responds to whatever map it is spawned in.
- [x] **Steering Behaviours:** Custom `CalculateWander` and `CalculateEvade` BT Tasks.
- [x] **Knowledge-Based Priorities:** - Prefers Medkits when bleeding out.
  - Recognises it cannot fight Heavy Zombies.
  - Throws away empty guns.
- [x] **Entity Management:** Correctly handles Food (Stamina), Medkits (Health), Weapons, Houses (Cover), and ignores Garbage.
- [ ] More todos later on

---

## 🔚 Conclusion & Takeaways
The greatest lesson learned during this project was dealing with **Logic Traps**. 
Explain more here...

---
*Developed by Ana Paoliello de Lucena Carvalho for the Algorithms 2 course at DAE (Howest)*
