// Actions, the night-shift job, customers and the three shifts.
#include "story_impl.h"
#include "prefab_util.h"

static Vector3 StoreP(float x, float y, float z) {
    Entity* st = Scn().Find("store");
    if (!st) return { x, y, z };
    return Vector3Transform({ x, y, z }, MatPose(st->base, st->worldYaw));
}

// ---------------------------------------------------------------------------
// Actions
// ---------------------------------------------------------------------------
void Story::Impl::RegisterActions() {
    Game& gg = g;
    auto& A = gg.actions;
    A.clear();
    OnEntityEvent = [](const std::string& ev, Entity& e, Vector3 p) {
        if (ev == "door_latch") audio::Play3D("door_latch", p, 0.7f);
        (void)e;
    };
    A["door"] = { [&gg](Entity& e) -> std::string {
                      if (e.state["locked"] > 0.5f) return "Locked";
                      return e.state["open"] > 0.5f ? "Close" : "Open";
                  },
                  [&gg](Entity& e) {
                      if (e.state["locked"] > 0.5f) {
                          audio::Play3D("door_locked", e.FocusPoint(), 0.8f);
                          gg.hud.Hint(e.prefab == "door_iron" ? "It won't move. There's a keyhole the size of a thumb." : "It's locked.", 3.0f);
                          return;
                      }
                      bool open = e.state["open"] < 0.5f;
                      e.state["open"] = open ? 1.0f : 0.0f;
                      std::string kind = e.prefab == "door_glass" ? "door_glass" : (e.prefab == "door_metal" || e.prefab == "door_iron" ? "door_metal" : "door_wood");
                      if (e.prefab == "door_iron") { audio::Play3D("iron_door", e.FocusPoint(), 1.0f); return; }
                      if (kind == "door_glass") audio::Play3D(open ? "door_glass_open" : "door_latch", e.FocusPoint(), 0.9f);
                      else audio::Play3D((kind + (open ? "_open" : "_close")).c_str(), e.FocusPoint(), 0.9f);
                  } };
    A["talk_grethnar"] = { [&gg](Entity&) -> std::string { return gg.Flag("can_talk") || gg.chapter == CH_AWAKENING ? "Talk" : ""; },
                           [&gg](Entity&) { gg.SetFlag("talk_request"); } };
    A["wreck"] = { [](Entity&) -> std::string { return "Look inside"; },
                   [&gg](Entity&) {
                       if (!gg.Flag("saw_body")) { gg.SetFlag("saw_body"); gg.hud.Say("ADAM", "That's... me.", 2.5f); gg.player.fear = 0.6f; }
                       else gg.hud.Hint("You don't want to look again.", 2.5f);
                   } };
    A["hatch"] = { [this, &gg](Entity&) -> std::string {
                       if (Task("tank") && !Task("tank")->done) return "Dip the tank";
                       return "Inspect the hatch";
                   },
                   [this, &gg](Entity& e) {
                       if (JobTask* t = Task("tank"); t && !t->done) {
                           audio::Play3D("metal_hit", e.FocusPoint(), 0.6f);
                           gg.story->side.Wait(1.2f).Do([&gg]() { audio::Play("drip", 0.8f); })
                               .Wait(1.0f).Do([this, &gg]() {
                                   gg.hud.Say("ADAM", "The dipstick's wet. There's hair on it. Long, dark hair.", 3.5f);
                                   gg.player.fear = 0.7f;
                                   CompleteTask("tank");
                               });
                           return;
                       }
                       if (!gg.Flag("hatch_seen")) {
                           gg.SetFlag("hatch_seen");
                           gg.hud.Say("ADAM", "The marks stop here. The metal is warm.", 3.0f);
                           gg.story->side.Wait(3.2f).Do([&gg]() { audio::Play("breath_out", 0.4f, 0.7f); gg.hud.Say("", "(something underneath is breathing)", 3.0f); });
                       } else gg.hud.Hint("Warm. Breathing.", 2.0f);
                   } };
    A["payphone"] = { [this](Entity&) -> std::string { return phoneRingT >= 0 ? "Answer" : "Use phone"; },
                      [this, &gg](Entity& e) {
                          if (phoneRingT < 0) { audio::Play3D("phone_pickup", e.FocusPoint(), 0.6f); gg.hud.Hint("Dial tone. You don't remember any numbers.", 3.0f); return; }
                          phoneRingT = -1.0f;
                          audio::StopAll("phone_ring");
                          audio::Play3D("phone_pickup", e.FocusPoint(), 0.8f);
                          phoneCalls++;
                          gg.player.locked = true;
                          Script& sd = gg.story->side;
                          if (phoneCalls == 1) {
                              sd.Do([]() { audio::Amb().whispers = 0.25f; })
                                .Say("", "(static)", 2.0f).Say("", "(breathing. close to the mouthpiece.)", 3.0f)
                                .Say("VOICE", "Don't come looking.", 2.5f)
                                .Do([&gg]() { audio::Amb().whispers = 0.0f; audio::Play("phone_pickup", 0.6f); gg.player.locked = false; gg.SetFlag("phone_answered_1"); });
                          } else if (phoneCalls == 2) {
                              sd.Say("", "(rain on the line, as if the caller is standing outside)", 3.0f)
                                .Say("WOMAN", "Zain? Baby, is that you?", 3.0f)
                                .Say("WOMAN", "Tell your brother to go home.", 3.0f)
                                .Do([&gg]() { audio::Play("phone_pickup", 0.6f); gg.player.locked = false; gg.SetFlag("phone_answered_2"); gg.player.fear = 0.6f; });
                          } else {
                              sd.Say("", "(a long, wet scraping, like something being dragged)", 3.5f)
                                .Do([&gg]() { audio::Play("phone_pickup", 0.6f); gg.player.locked = false; });
                          }
                      } };
    A["pump"] = { [this, &gg](Entity& e) -> std::string {
                      for (auto& c : customers)
                          if (c.state == 1 && ("pump_" + std::to_string(c.pump)) == e.name) return gg.held == "nozzle" ? "" : "Take the nozzle";
                      return "";
                  },
                  [this, &gg](Entity& e) {
                      gg.held = "nozzle";
                      audio::Play3D("nozzle_lift", e.FocusPoint(), 0.8f);
                      for (auto& c : customers) if (c.state == 1) { c.state = 2; c.pumped = 0; }
                  } };
    A["register"] = { [this, &gg](Entity&) -> std::string {
                          if (gg.Flag("customer_at_counter")) return "Ring them up";
                          return gg.money >= 4.0f ? "Buy batteries  ($4.00)" : "Batteries $4.00 (not enough)";
                      },
                      [this, &gg](Entity& e) {
                          if (gg.Flag("customer_at_counter")) {
                              gg.flags.erase("customer_at_counter");
                              gg.story->side.Do([&e]() { audio::Play3D("register_key", e.FocusPoint(), 0.7f); })
                                  .Wait(0.25f).Do([&e]() { audio::Play3D("register_key", e.FocusPoint(), 0.7f); })
                                  .Wait(0.3f).Do([&e]() { audio::Play3D("register_drawer", e.FocusPoint(), 0.8f); })
                                  .Wait(0.6f).Do([this, &e]() { audio::Play3D("receipt", e.FocusPoint(), 0.6f); Pay(6.40f, "wages"); CompleteTask("register"); g.SetFlag("customer_leave"); });
                              return;
                          }
                          if (gg.money >= 4.0f) {
                              gg.money -= 4.0f;
                              gg.player.battery = 1.0f;
                              audio::Play3D("register_drawer", e.FocusPoint(), 0.7f);
                              gg.hud.Notify("- $4.00  batteries (flashlight full)");
                          } else audio::Play3D("ui_back", e.FocusPoint(), 0.5f);
                      } };
    A["mop"] = { [this, &gg](Entity&) -> std::string {
                     JobTask* t = Task("mop");
                     if (!t || t->done) return "";
                     return gg.held == "mop" ? "Put the mop back" : (gg.held.empty() ? "Take the mop" : "");
                 },
                 [&gg](Entity& e) {
                     gg.held = gg.held == "mop" ? "" : "mop";
                     audio::Play3D("pickup", e.FocusPoint(), 0.6f);
                     if (gg.held == "mop") gg.hud.Hint("Find the stains. Hold E to scrub.", 3.0f);
                 } };
    A["stock_box"] = { [&gg](Entity&) -> std::string { return gg.held.empty() ? "Pick up the stock box" : ""; },
                       [&gg](Entity& e) {
                           gg.held = "stock_box";
                           e.visible = false; e.interact.enabled = false;
                           Phys().SetOwnerEnabled(e.id, false);
                           audio::Play3D("pickup", e.FocusPoint(), 0.7f);
                           gg.hud.Hint("Take it to an empty shelf.", 2.5f);
                       } };
    A["shelf"] = { [&gg](Entity& e) -> std::string { return gg.held == "stock_box" && e.state["stocked"] < 0.5f ? "Restock the shelf" : ""; },
                   [this, &gg](Entity& e) {
                       gg.held = "";
                       e.state["stocked"] = 1.0f;
                       audio::Play3D("drop_box", e.FocusPoint(), 0.6f);
                       audio::Play3D("paper", e.FocusPoint(), 0.5f);
                       if (JobTask* t = Task("restock")) { t->count++; if (t->count >= t->need) CompleteTask("restock"); else gg.hud.Notify(TextFormat("restocked %d/%d", t->count, t->need)); }
                   } };
    A["trash_bag"] = { [this, &gg](Entity&) -> std::string { return gg.held.empty() ? "Take the trash out" : ""; },
                       [&gg](Entity& e) {
                           gg.held = "trash_bag"; e.visible = false; e.interact.enabled = false;
                           audio::Play3D("pickup", e.FocusPoint(), 0.7f);
                           gg.hud.Hint("The dumpster is out back.", 2.5f);
                       } };
    A["dumpster"] = { [&gg](Entity&) -> std::string { return gg.held == "trash_bag" ? "Throw the bag in" : "Dumpster"; },
                      [this, &gg](Entity& e) {
                          if (gg.held != "trash_bag") { gg.hud.Hint("It smells like the field.", 2.0f); return; }
                          gg.held = "";
                          audio::Play3D("drop_box", e.FocusPoint(), 0.9f, 0.8f);
                          CompleteTask("trash");
                          if (shift == 1) {
                              // something inside answers
                              gg.story->side.Wait(1.6f).Do([&e, &gg]() {
                                  audio::Play3D("slam", e.FocusPoint(), 0.9f);
                                  gg.ShakeCamera(0.25f);
                                  gg.player.fear = 0.9f;
                                  gg.hud.Say("", "(something inside the dumpster moves)", 2.5f);
                              }).Wait(2.0f).Do([&e]() { audio::Play3D("breath_out", e.FocusPoint(), 0.5f, 0.6f); });
                          }
                      } };
    A["breaker"] = { [&gg](Entity&) -> std::string { return gg.Flag("power_out") ? "Reset the breakers" : "Breaker panel"; },
                     [this, &gg](Entity& e) {
                         if (!gg.Flag("power_out")) { audio::Play3D("switch", e.FocusPoint(), 0.5f); gg.hud.Hint("Everything's on. For now.", 2.0f); return; }
                         Script& sd = gg.story->side;
                         for (int i = 0; i < 3; i++) sd.Do([&e]() { audio::Play3D("breaker", e.FocusPoint(), 0.9f); }).Wait(0.45f);
                         sd.Do([this, &gg]() {
                             gg.flags.erase("power_out");
                             for (const char* grp : { "store", "canopy", "station", "open_sign", "store_sign", "storage" }) { Scn().SetLightGroup(grp, true); Scn().SetLightGroupFlicker(grp, 0.9f); }
                             audio::Play("switch", 0.6f);
                             CompleteTask("breaker");
                         }).Wait(1.5f).Do([]() {
                             for (const char* grp : { "store", "canopy", "station", "store_sign", "storage" }) Scn().SetLightGroupFlicker(grp, 0.0f);
                             Scn().SetLightGroupFlicker("open_sign", 0.25f);
                         });
                     } };
    RegisterCollegeActions(*this);
}

// ---------------------------------------------------------------------------
// Tasks
// ---------------------------------------------------------------------------
JobTask* Story::Impl::Task(const std::string& id) {
    for (auto& t : tasks) if (t.id == id) return &t;
    return nullptr;
}
bool Story::Impl::AllTasksDone() const {
    for (auto& t : tasks) if (!t.done) return false;
    return !tasks.empty();
}
void Story::Impl::CompleteTask(const std::string& id) {
    JobTask* t = Task(id);
    if (!t || t->done) return;
    t->done = true;
    audio::Play("paper", 0.35f);
    if (g.chapter <= CH_SHIFT3) Pay(id == "pump" ? 0.0f : 3.25f, "wages");
    g.hud.Notify("done: " + t->label);
    if (AllTasksDone() && g.chapter <= CH_SHIFT3 && id != "boiler") g.hud.Objective("Tell Grethnar you're done.");
}

void Story::Impl::SpawnStains(int n) {
    if (!stainModel) {
        Decals d;
        d.Pool({ 0, 50, 0 }, 0.45f, MAT_BLOOD, 5);
        // rebuild the pool around the origin so it can be instanced
        ModelBuilder mb;
        MeshBuilder& m = mb.M(MAT_BLOOD);
        const int segs = 24;
        for (int i = 0; i < segs; i++) {
            float a0 = (float)i / segs * 2 * PI, a1 = (float)(i + 1) / segs * 2 * PI;
            float r0 = 0.45f * (0.7f + 0.4f * (Fbm2(cosf(a0) * 1.5f, sinf(a0) * 1.5f, 3, 0, 5) * 0.5f + 0.5f));
            float r1 = 0.45f * (0.7f + 0.4f * (Fbm2(cosf(a1) * 1.5f, sinf(a1) * 1.5f, 3, 0, 5) * 0.5f + 0.5f));
            m.TriN({ 0, 0.004f, 0 }, { cosf(a1) * r1, 0.004f, sinf(a1) * r1 }, { cosf(a0) * r0, 0.004f, sinf(a0) * r0 }, { 0, 1, 0 }, { 0, 1, 0 }, { 0, 1, 0 });
        }
        stainModel = mb.Build(false);
    }
    stains.clear(); stainAlpha.clear();
    Rng r((uint32_t)(shift * 97 + 13));
    for (int i = 0; i < n; i++) {
        // aisles of the store (local), avoiding shelves
        float xs[] = { -6.6f, -4.4f, -2.0f, 0.6f };
        float x = xs[r.RangeI(0, 3)] + r.Range(-0.3f, 0.3f);
        float z = r.Range(-4.5f, 5.0f);
        stains.push_back(StoreP(x, 0.151f, z));
        stainAlpha.push_back(1.0f);
    }
    stainsLeft = n;
}

void Story::Impl::StartShiftTasks(int n) {
    tasks.clear();
    shift = n;
    shiftT = 0;
    // restock boxes in the storage room
    for (Entity* e : Scn().FindAll("stock_box")) Scn().Remove(e);
    for (Entity* e : Scn().FindAll("trash_bag")) Scn().Remove(e);
    for (Entity* e : Scn().FindAll("store_shelf")) e->state["stocked"] = 0;
    auto addBoxes = [](int k) {
        for (int i = 0; i < k; i++) Scn().Spawn("stock_box", { 3.4f + i * 0.55f, 0.15f, -3.3f + (i % 2) * 0.3f }, { (float)(i * 23), 0, 0 }, { { "parent", "store" }, { "tag", "runtime" } });
    };
    if (n == 1) {
        tasks = { { "pump", "fill up the customer at the pumps" }, { "restock", "restock two shelves", false, 0, 2 },
                  { "mop", "mop the stains on the floor" }, { "trash", "take the trash out to the dumpster" } };
        addBoxes(2);
        SpawnStains(3);
        Scn().Spawn("trash_bag", { 5.6f, 0.15f, 1.3f }, { 20, 0, 0 }, { { "parent", "store" }, { "tag", "runtime" } });
        g.story->side.Wait(25.0f).Do([this]() { SpawnCustomer(false); });
        g.story->side.Until([this]() { return Task("pump") && Task("pump")->done; }).Wait(40.0f).Do([this]() { SpawnCustomer(true); });
    } else if (n == 2) {
        tasks = { { "pump", "fill up the customer at the pumps" }, { "register", "ring up the customer inside" },
                  { "tank", "dip the underground tank" }, { "ledger", "fetch the ledger from blackwood college" } };
        g.story->side.Wait(15.0f).Do([this]() { SpawnCustomer(false); });
        g.story->side.Wait(20.0f).Do([this]() { g.SetFlag("spawn_walkin"); });
        g.story->side.Until([this]() { return g.Flag("ledger_taken"); }).Wait(8.0f).Do([this]() { phoneRingT = 0; });
    } else {
        tasks = { { "mop", "mop the stains on the floor" }, { "breaker", "keep the lights on" } };
        SpawnStains(4);
        g.story->side.Wait(35.0f).Do([this]() {
            g.SetFlag("power_out");
            for (const char* grp : { "store", "canopy", "station", "open_sign", "store_sign", "storage" }) Scn().SetLightGroup(grp, false);
            audio::Play("breaker", 0.9f, 0.7f);
            audio::Play("stinger_low", 0.4f);
            g.hud.Hint("The power's gone. The breaker panel is in the storage room.", 4.0f);
            g.player.fear = 0.6f;
        });
    }
    std::vector<std::pair<std::string, bool>> list;
    for (auto& t : tasks) list.push_back({ t.label, t.done });
    g.hud.SetTaskList(list);
}

// ---------------------------------------------------------------------------
// Per-frame job logic: scrubbing, pumping, the walk-in customer
// ---------------------------------------------------------------------------
void Story::Impl::UpdateJob(float dt) {
    if (tasks.empty()) return;
    shiftT += dt;
    std::vector<std::pair<std::string, bool>> list;
    for (auto& t : tasks) list.push_back({ t.label + (t.need > 1 && !t.done ? TextFormat(" (%d/%d)", t.count, t.need) : ""), t.done });
    g.hud.SetTaskList(list);
    // scrubbing stains with the mop
    if (g.held == "mop" && !g.story->InCutscene()) {
        Vector3 eye = g.player.EyePos(), fwd = g.player.Forward();
        int best = -1; float bestD = 1e9f;
        for (size_t i = 0; i < stains.size(); i++) {
            if (stainAlpha[i] <= 0.0f) continue;
            float d = Vector3Distance(g.player.feet, stains[i]);
            Vector3 to = Vector3Normalize(Vector3Subtract(stains[i], eye));
            if (d < 2.2f && Vector3DotProduct(to, fwd) > 0.75f && d < bestD) { bestD = d; best = (int)i; }
        }
        if (best >= 0) {
            g.hud.SetFocus(true);
            g.hud.SetPrompt("Hold  Scrub", 1.0f - stainAlpha[best]);
            if (IsKeyDown(KEY_E) || IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) {
                stainAlpha[best] -= dt / 2.2f;
                if (fmodf(shiftT, 0.45f) < dt) audio::Play3D("squelch", stains[best], 0.35f, 1.3f);
                g.player.shake = fmaxf(g.player.shake, 0.03f);
                if (stainAlpha[best] <= 0.0f) {
                    stainAlpha[best] = 0.0f;
                    stainsLeft--;
                    if (stainsLeft <= 0) {
                        CompleteTask("mop");
                        g.held = "";
                        if (shift == 3) {
                            // the last one comes back as words
                            Vector3 p = stains[best];
                            g.story->side.Wait(2.5f).Do([this, p]() {
                                int m = SignMaterial("he_lies", "HE LIES", ui::F_MONO_BOLD, 120, Color{ 35, 0, 0, 255 }, Color{ 95, 12, 10, 255 }, 512, 170, 0.9f);
                                ModelBuilder mb;
                                mb.Push(); mb.Translate(p); mb.RotateX(-90);
                                mb.M(m).BoxUV({ 0, 0, 0.006f }, { 1.6f, 0.53f, 0.002f });
                                mb.Pop();
                                decals.models.push_back(mb.Build(false)); decals.visible.push_back(true); decals.alpha.push_back(1);
                                audio::Play3D("squelch", p, 0.8f, 0.6f);
                                g.hud.Hint("It came back. It spells something.", 4.0f);
                                g.player.fear = 0.9f;
                            });
                        }
                    }
                }
            }
        }
    }
    // pumping fuel into a waiting customer's car
    for (auto& c : customers) {
        if (c.state != 2 || g.held != "nozzle") continue;
        Entity* pump = Scn().Find("pump_" + std::to_string(c.pump));
        if (!pump) continue;
        float d = Vector3Distance(g.player.feet, pump->base);
        bool holding = (IsKeyDown(KEY_E) || IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) && d < 3.2f;
        auto& a = audio::Amb();
        if (holding) {
            float rate = c.pumped > c.target - 1.0f ? 0.9f : 2.6f;   // it slows near the target... or you just notice more
            c.pumped += rate * dt;
            a.pumpMotor = 0.6f; a.fuelFlow = 0.5f;
            if (fmodf(c.pumped, 0.25f) < rate * dt) audio::Play3D("register_key", pump->FocusPoint(), 0.08f, 2.2f);
            c.t = 0;
        } else {
            a.pumpMotor = Damp(a.pumpMotor, 0.0f, 6.0f, dt); a.fuelFlow = Damp(a.fuelFlow, 0.0f, 8.0f, dt);
            if (c.pumped > 0.5f) c.t += dt;
        }
        g.hud.SetFocus(true);
        g.hud.SetPrompt(TextFormat("Hold  Pump   $%.2f  of  $%.2f", c.pumped, c.target), Saturate(c.pumped / c.target));
        if (c.pumped > 0.5f && !holding && c.t > 1.6f) {
            // done: hang up, get paid
            audio::Play3D("pump_shutoff", pump->FocusPoint(), 0.8f);
            audio::Play3D("nozzle_hang", pump->FocusPoint(), 0.8f);
            g.held = "";
            float diff = c.pumped - c.target;
            if (fabsf(diff) < 0.1f) { Pay(c.target + 2.0f, "exact. they tip you"); g.hud.Hint("Right on the cent.", 2.0f); }
            else if (diff < 0) { Pay(c.pumped, "short-changed them"); }
            else { Pay(c.target, "they won't pay the extra"); }
            c.state = 3; c.t = 0;
            a.pumpMotor = 0; a.fuelFlow = 0;
            CompleteTask("pump");
        }
    }
    // walk-in customer for the register (night two)
    if (g.Flag("spawn_walkin") && !g.A("walkin")) {
        g.flags.erase("spawn_walkin");
        Actor* w = g.SpawnActor("walkin", SpecCustomer(71), StoreP(-0.4f, 0.15f, 8.5f), 0);
        (void)w;
        g.SetFlag("walkin_entering");
    }
    if (Actor* w = g.A("walkin")) {
        static float wt = 0;
        wt += dt;
        Vector3 door = StoreP(-0.4f, 0.15f, 7.2f), counter = StoreP(3.2f, 0.15f, 4.2f), outside = StoreP(-0.4f, 0.15f, 12.0f);
        Entity* front = Scn().Find("store_front_door");
        auto walkTo = [&](Vector3 to) {
            Vector3 d = Vector3Subtract(to, w->pos); d.y = 0;
            float len = Vector3Length(d);
            if (len < 0.1f) { w->SetPose(PoseStand(wt)); return true; }
            Vector3 step = Vector3Scale(d, fminf(1.2f * dt, len) / len);
            w->pos = Vector3Add(w->pos, step);
            w->pos.y = StoreP(0, 0.15f, 0).y;
            w->yaw = atan2f(d.x, d.z);
            w->SetPose(PoseWalk(wt * 0.9f, 0.9f, 0));
            if (fmodf(wt, 0.55f) < dt) audio::PlayFootstep(SURF_TILE, w->pos, 0.4f, true);
            return false;
        };
        if (g.Flag("walkin_entering")) {
            if (front && Vector3Distance(w->pos, door) < 2.0f && front->state["open"] < 0.5f) { front->state["open"] = 1; audio::Play3D("door_glass_open", door, 0.8f); }
            if (walkTo(counter)) { g.flags.erase("walkin_entering"); g.SetFlag("customer_at_counter"); w->yaw = PI + StoreYawFix(); }
        } else if (g.Flag("customer_at_counter")) {
            w->SetPose(PoseStand(wt));
            w->lookAt = g.player.EyePos(); w->lookWeight = 1.0f;    // it watches you the whole time
        } else if (g.Flag("customer_leave")) {
            w->lookWeight = 0;
            if (walkTo(outside)) { g.RemoveActor("walkin"); g.flags.erase("customer_leave"); if (front) front->state["open"] = 0; }
        }
    }
}

float StoreYawFix() {
    Entity* st = Scn().Find("store");
    return st ? st->worldYaw : 0.0f;
}

// ---------------------------------------------------------------------------
// Customer cars
// ---------------------------------------------------------------------------
void Story::Impl::SpawnCustomer(bool ghost) {
    Customer c;
    c.ghost = ghost;
    c.pump = 3;
    c.target = ghost ? 0.0f : (float)GetRandomValue(8, 25);
    c.car.visible = true;
    const char* colors[] = { "white", "red", "green", "beige" };
    c.car.body = GetPrefabModel("sedan", { { "color", colors[GetRandomValue(0, 3)] }, { "state", "intact" } });
    c.car.pos = RoadPoint(-140.0f, 1.85f);
    c.car.yaw = RoadYaw(-140.0f);
    c.car.speed = 12.0f;
    c.car.interior = false;
    c.stopPos = { 16.0f, 0, 0.4f };
    c.stopPos.y = World().Height(c.stopPos.x, c.stopPos.z) + 0.05f;
    if (!ghost) {
        std::string name = "driver_" + std::to_string(customers.size());
        Actor* d = g.SpawnActor(name, SpecCustomer((uint32_t)customers.size() + 3u), c.car.pos, 0);
        d->SetPose(PoseSitDrive(0, 0), true);
        c.driver = name;
        g.hud.Hint("Headlights. A customer.", 2.5f);
        audio::Play("indicator", 0.4f);
    }
    customers.push_back(c);
}

void Story::Impl::UpdateCustomers(float dt) {
    auto& a = audio::Amb();
    float nearest = 1e9f, nearSpeed = 0;
    for (auto& c : customers) {
        CarRig& car = c.car;
        c.t += dt;
        if (c.state == 0) {
            // drive north, then swing into the forecourt and brake at the pumps
            float z = car.pos.z;
            if (z < -32.0f) {
                z += car.speed * dt;
                car.pos = RoadPoint(z, 1.85f);
                car.yaw = RoadYaw(z);
            } else {
                Vector3 to = Vector3Subtract(c.stopPos, car.pos); to.y = 0;
                float d = Vector3Length(to);
                float want = atan2f(to.x, to.z);
                if (d < 6.0f) want = 0.0f;
                car.yaw = DampAngle(car.yaw, want, 2.2f, dt);
                car.speed = Damp(car.speed, Clamp(d * 0.9f, 0.0f, 8.0f), 2.0f, dt);
                car.steer = Clamp(WrapAngle(want - car.yaw) * 2.0f, -1.0f, 1.0f);
                car.pos = Vector3Add(car.pos, Vector3Scale(YawDir(car.yaw), car.speed * dt));
                car.pos.y = SurfaceY(car.pos.x, car.pos.z, car.pos.y + 1.0f) + 0.02f;
                if (d < 0.35f && car.speed < 0.4f) {
                    car.speed = 0;
                    c.state = c.ghost ? 5 : 1;
                    c.t = 0;
                    audio::Play3D("car_door", car.pos, 0.3f);
                    if (!c.ghost) g.hud.Hint(TextFormat("They want $%.2f of regular. Pump %d.", c.target, c.pump), 4.0f);
                }
            }
        } else if (c.state == 3 && c.t > 2.5f) {
            // leave: back onto the road northbound
            Vector3 exit = RoadPoint(40.0f, 1.85f);
            Vector3 to = Vector3Subtract(exit, car.pos); to.y = 0;
            float d = Vector3Length(to);
            if (d > 2.0f) {
                car.yaw = DampAngle(car.yaw, atan2f(to.x, to.z), 1.8f, dt);
                car.speed = Damp(car.speed, 9.0f, 1.0f, dt);
                car.pos = Vector3Add(car.pos, Vector3Scale(YawDir(car.yaw), car.speed * dt));
                car.pos.y = SurfaceY(car.pos.x, car.pos.z, car.pos.y + 1.0f) + 0.02f;
            } else c.state = 4;
        } else if (c.state == 4) {
            float z = car.pos.z + car.speed * dt;
            car.speed = Damp(car.speed, 14.0f, 0.5f, dt);
            car.pos = RoadPoint(z, 1.85f);
            car.yaw = RoadYaw(z);
            if (z > 125.0f) { car.visible = false; c.state = 9; if (!c.driver.empty()) g.RemoveActor(c.driver); }
        } else if (c.state == 5) {
            // the ghost car: engine running, nobody inside. When you get close, the horn - then it rolls away.
            float d = Vector3Distance(g.player.feet, car.pos);
            if (d < 4.5f && c.t > 1.0f && !g.Flag("ghost_horn")) {
                g.SetFlag("ghost_horn");
                audio::Play3D("horn", car.pos, 1.0f);
                g.player.fear = 1.0f;
                g.ShakeCamera(0.2f);
                g.hud.Say("ADAM", "There's no one in it.", 2.5f);
                c.state = 3; c.t = 0;
            }
            if (c.t > 45.0f) { c.state = 3; c.t = 0; }
        }
        if (!c.driver.empty()) if (Actor* d = g.A(c.driver)) {
            d->pos = car.Local({ kDriverX, kSeatY + 0.1f - d->model->hipHeight, kSeatZ });
            d->yaw = car.yaw;
            d->SetPose(PoseSitDrive(c.t, car.steer));
            if (c.state == 1 || c.state == 2) { d->lookAt = g.player.EyePos(); d->lookWeight = 0.9f; }
            else d->lookWeight = 0;
        }
        float dist = Vector3Distance(car.pos, g.player.feet);
        if (car.visible && dist < nearest) { nearest = dist; nearSpeed = car.speed; }
    }
    if (g.chapter >= CH_SHIFT1 && g.chapter <= CH_SHIFT3) {
        a.engine = Damp(a.engine, nearest < 50.0f ? Saturate(1.0f - nearest / 50.0f) * 0.6f : 0.0f, 3.0f, dt);
        a.rpm = 800 + nearSpeed * 150;
        a.road = Damp(a.road, nearest < 50.0f ? Saturate(1.0f - nearest / 50.0f) * nearSpeed / 14.0f * 0.5f : 0.0f, 3.0f, dt);
    }
}

// ---------------------------------------------------------------------------
// The shifts
// ---------------------------------------------------------------------------
void Story::Impl::BuildShift(Script& s, int n) {
    Game& gg = g;
    const char* sub[] = { "", "your first shift at grethnar's", "rain", "fog" };
    s.Do([this, &gg, n, sub]() {
        gg.story->cutscene = false;
        gg.hud.FadeInstant(1.0f);
        gg.hud.FadeTo(0.0f, 0.5f);
        gg.hud.Title(n == 1 ? "NIGHT ONE" : (n == 2 ? "NIGHT TWO" : "NIGHT THREE"), sub[n], 5.0f);
        // start each night inside the store, by the counter
        gg.player.Spawn(StoreP(1.5f, 0.3f, 2.5f), StoreYawFix() * RAD2DEG + 180.0f);
        gg.player.flashOn = false;
        gg.flags.erase("can_talk");
        wreckLights = false;
        StartShiftTasks(n);
        gg.hud.Objective("Tonight's work. Hold Tab to see the list.");
        audio::Play("door_latch", 0.3f);
    });
    s.Until([this]() { return AllTasksDone() || (shift == 2 && Task("ledger") && Task("ledger")->done && Task("pump")->done && Task("register")->done && Task("tank")->done); });
    s.Do([&gg]() { gg.SetFlag("can_talk"); gg.hud.Objective("Tell Grethnar you're done."); });
    if (n == 3) {
        // night three: the errand that leads under the college
        s.Until([&gg]() { return gg.Flag("talk_request"); });
        s.Do([&gg]() { gg.flags.erase("talk_request"); gg.flags.erase("can_talk"); gg.player.locked = true; });
        s.Say("GRETHNAR", "Lights are back. Good. One more thing tonight.")
         .Say("GRETHNAR", "The college boiler. Down in the basement. Relight the pilot for me.")
         .Say("ADAM", "Why does a dead college need heat?")
         .Say("GRETHNAR", "It isn't for the college.", 2.5f)
         .Do([this, &gg]() { gg.player.locked = false; tasks.push_back({ "boiler", "relight the boiler under blackwood college" }); gg.hud.Objective("Relight the boiler in the college basement."); });
        BuildCollegeDiscovery(*this, s);
        return;
    }
    s.Until([&gg]() { return gg.Flag("talk_request"); });
    s.Do([&gg]() { gg.flags.erase("talk_request"); gg.flags.erase("can_talk"); gg.player.locked = true; gg.hud.Objective(""); });
    if (n == 1) {
        s.Say("GRETHNAR", "Not bad. Not bad at all. You're getting heavier, you know. I can hear you walk.")
         .Say("GRETHNAR", "Here's your true thing.")
         .Do([&gg]() { audio::Play("pickup", 0.8f); gg.GiveItem("sneaker"); gg.hud.Notify("received: a child's sneaker"); })
         .Say("GRETHNAR", "Left shoe. Found it by my tank this morning.")
         .Say("ADAM", "That's his. That's Zain's.")
         .Say("GRETHNAR", "He's alive, if that helps you sleep.", 2.6f)
         .Say("GRETHNAR", "You don't sleep anymore, do you.", 2.6f, 1.2f);
    } else {
        s.Say("GRETHNAR", "My ledger. Good boy. Let me see... yes. Here she is.")
         .Say("ADAM", "Who?")
         .Say("GRETHNAR", "Sundays. Orange sweets. Sixty cents. Lovely woman, your mother.")
         .Say("GRETHNAR", "The ground kept her too, you know. It keeps everything that's given to it.")
         .Say("GRETHNAR", "And this was in your brother's pocket. Or near enough.")
         .Do([this, &gg]() { audio::Play("paper", 0.8f); gg.GiveItem("polaroid"); showPolaroid = 6.0f; })
         .Wait(6.5f)
         .Say("ADAM", "That's you. In the window. Behind them.", 3.0f, 1.0f);
    }
    s.Do([&gg]() { gg.player.locked = false; gg.hud.FadeTo(1.0f, 0.5f); audio::Play("stinger_low", 0.4f); });
    s.Wait(2.5f);
    s.Do([&gg, n]() { gg.story->pendingChapter = CH_SHIFT1 + n; });
}
