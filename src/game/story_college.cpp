// College errands, the hidden room and the old man; the cow field and the key.
#include "story_impl.h"
#include "prefab_util.h"

Vector3 CollegeP(float x, float y, float z) {
    Entity* c = Scn().Find("college");
    if (!c) return { x, y, z };
    return Vector3Transform({ x, y, z }, MatPose(c->base, c->worldYaw));
}
static float CollegeYaw(float lx, float lz) {
    Vector3 a = CollegeP(0, 0, 0), b = CollegeP(lx, 0, lz);
    return atan2f(b.x - a.x, b.z - a.z);
}
static Vector3 ToCollege(Vector3 w) {
    Entity* c = Scn().Find("college");
    if (!c) return w;
    return Vector3Transform(w, MatrixInvert(MatPose(c->base, c->worldYaw)));
}
static void HideEntity(Entity* e) {
    if (!e) return;
    e->visible = false;
    e->interact.enabled = false;
    Phys().SetOwnerEnabled(e->id, false);
}
static bool InHiddenRoom(Vector3 feet) {
    Vector3 l = ToCollege(feet);
    return l.x > 0.1f && l.x < 4.9f && l.z > -10.9f && l.z < -4.1f && l.y < -1.5f;
}

// The old man sits on the edge of the bed, facing the way you came in.
static Actor* SpawnOldMan(Story::Impl& im, bool dead) {
    Game& g = im.g;
    Actor* a = g.SpawnActor("oldman", SpecOldMan(), CollegeP(3.6f, -3.0f, -5.5f), CollegeYaw(0.3f, -1.0f) * RAD2DEG);
    a->pos = CollegeP(3.6f, -3.0f + 0.47f + 0.08f - a->model->hipHeight, -5.5f);
    a->SetPose(dead ? PoseSlumped(0) : PoseSitPassenger(0, 0), true);
    if (!dead) { g.actors["oldman"].action = "talk_oldman"; g.actors["oldman"].radius = 2.6f; }
    return a;
}

// ---------------------------------------------------------------------------
// Actions: ledger, boiler, bookshelf, the old man, the iron door, shovel, cows
// ---------------------------------------------------------------------------
void RegisterCollegeActions(Story::Impl& im) {
    Game& gg = im.g;
    auto& A = gg.actions;
    Story::Impl* pim = &im;
    int ch = gg.chapter;

    // world state that depends on how far the story has come
    if (ch > CH_SHIFT2) HideEntity(Scn().Find("ledger"));
    if (Entity* bs = Scn().Find("bookshelf")) bs->state["open"] = ch >= CH_KEY ? 1.0f : 0.0f;
    Scn().SetLightGroup("boiler_flame", ch >= CH_KEY);
    Scn().SetLightGroup("basement", ch < CH_KEY);
    Scn().SetLightGroup("candles", ch <= CH_KEY);
    if (Entity* d = Scn().Find("iron_door")) {
        d->state["locked"] = ch >= CH_BELOW ? 0.0f : 1.0f;
        d->state["open"] = ch >= CH_BELOW ? 1.0f : 0.0f;
    }
    if (ch >= CH_KEY && ch < CH_BURN) SpawnOldMan(im, true);
    if (Entity* c3 = Scn().Find("cow_3")) { bool gone = ch > CH_KEY; c3->visible = !gone; c3->interact.enabled = !gone; Phys().SetOwnerEnabled(c3->id, !gone); }
    if (Entity* sh = Scn().Find("shovel")) { bool here = ch == CH_KEY; sh->visible = here || ch < CH_KEY; sh->interact.enabled = here; }

    A["ledger"] = { [pim](Entity&) -> std::string {
                        JobTask* t = pim->Task("ledger");
                        return t && !t->done ? "Take the ledger" : "";
                    },
                    [pim, &gg](Entity& e) {
                        HideEntity(&e);
                        audio::Play3D("paper", e.FocusPoint(), 0.8f);
                        gg.GiveItem("ledger");
                        gg.SetFlag("ledger_taken");
                        pim->CompleteTask("ledger");
                        Script& sd = gg.story->side;
                        sd.Say("ADAM", "Names. Dates. Amounts, in pencil. Every page a family from around here.", 4.0f)
                          .Say("ADAM", "The last line is fresh. \"Route 9. One of the same blood.\" Tonight's date.", 4.5f)
                          .Wait(1.0f)
                          .Do([&gg]() {
                              // the office door shuts behind you
                              if (Entity* d = Scn().Find("office_door")) {
                                  d->state["open"] = 0;
                                  audio::Play3D("slam", d->FocusPoint(), 1.0f);
                              }
                              gg.ShakeCamera(0.2f);
                              gg.player.fear = 0.9f;
                          });
                    } };

    A["boiler"] = { [pim, &gg](Entity&) -> std::string {
                        JobTask* t = pim->Task("boiler");
                        if (t && !t->done) return "Relight the pilot";
                        return gg.Flag("boiler_lit") || gg.chapter >= CH_KEY ? "" : "Boiler";
                    },
                    [pim, &gg](Entity& e) {
                        JobTask* t = pim->Task("boiler");
                        if (!t || t->done) { gg.hud.Hint("Cold iron. Something inside it ticks as it cools.", 3.0f); return; }
                        Vector3 p = e.FocusPoint();
                        gg.story->side.Do([p]() { audio::Play3D("match", p, 0.7f); })
                            .Wait(0.8f).Do([p, &gg]() {
                                audio::Play3D("fire_ignite", p, 0.9f);
                                Scn().SetLightGroup("boiler_flame", true);
                                gg.ShakeCamera(0.08f);
                            })
                            .Wait(0.6f).Do([pim, &gg]() { pim->CompleteTask("boiler"); gg.SetFlag("boiler_lit"); });
                    },
                    1.2f };

    A["bookshelf"] = { [&gg](Entity& e) -> std::string {
                           if (e.state["open"] > 0.5f) return "";
                           return gg.Flag("scratch_heard") ? "Push the bookshelf" : "Bookshelf";
                       },
                       [&gg](Entity& e) {
                           if (!gg.Flag("scratch_heard")) { gg.hud.Hint("Old textbooks, swollen with damp. The floor in front of it is scored.", 3.5f); return; }
                           e.state["open"] = 1.0f;
                           audio::Play3D("drag_short", e.FocusPoint(), 1.0f, 0.7f);
                           gg.story->side.Wait(0.7f).Do([&e]() { audio::Play3D("drag_short", e.FocusPoint(), 0.9f, 0.6f); });
                           gg.SetFlag("bookshelf_open");
                           gg.player.fear = 0.7f;
                       },
                       0.8f };

    A["talk_oldman"] = { [&gg](Entity&) -> std::string { return gg.Flag("oldman_done") || gg.Flag("talk_oldman") ? "" : "Talk"; },
                         [&gg](Entity&) { gg.SetFlag("talk_oldman"); } };

    // the iron door takes the key; otherwise it behaves like any door
    ActionHandler baseDoor = A["door"];
    A["door"] = { [&gg, baseDoor](Entity& e) -> std::string {
                      if (e.prefab == "door_iron" && e.state["locked"] > 0.5f && gg.Item("iron_key")) return "Use the key";
                      return baseDoor.prompt(e);
                  },
                  [&gg, baseDoor](Entity& e) {
                      if (e.prefab == "door_iron" && e.state["locked"] > 0.5f && gg.Item("iron_key")) {
                          audio::Play3D("keys", e.FocusPoint(), 0.8f);
                          gg.story->side.Wait(0.7f).Do([&e]() { audio::Play3D("unlock", e.FocusPoint(), 1.0f, 0.7f); });
                          e.state["locked"] = 0.0f;
                          gg.SetFlag("iron_unlocked");
                          return;
                      }
                      baseDoor.use(e);
                  } };

    A["shovel"] = { [&gg](Entity&) -> std::string { return gg.held.empty() ? "Take the shovel" : ""; },
                    [&gg](Entity& e) {
                        gg.held = "shovel";
                        HideEntity(&e);
                        audio::Play3D("pickup", e.FocusPoint(), 0.8f, 0.8f);
                        gg.hud.Notify("shovel");
                    } };

    A["cow"] = { [&gg](Entity& e) -> std::string {
                     if (e.name == "cow_3" && gg.held == "shovel" && !gg.Flag("key_dug")) return "";   // the dig prompt takes over
                     return "Look";
                 },
                 [&gg](Entity& e) {
                     static const char* order[] = { "", "The first one from the fence.", "The second from the fence.",
                                                    "The third from the fence. The earth beside her has been turned over.",
                                                    "The fourth.", "The fifth.", "Too far from the others. It's looking back at the road." };
                     int n = e.name.size() > 4 ? e.name[4] - '0' : 0;
                     if (n < 0 || n > 6) n = 0;
                     if (gg.chapter == CH_KEY) gg.hud.Hint(n ? order[n] : "Bones.", 3.5f);
                     else gg.hud.Hint("Picked clean. The grass won't grow where they lie.", 3.0f);
                     audio::Play3D("rattle_bones", e.FocusPoint(), 0.25f, 1.2f);
                 } };

    A["trough"] = { [](Entity&) -> std::string { return "Trough"; },
                    [&gg](Entity& e) {
                        audio::Play3D("drip", e.FocusPoint(), 0.6f);
                        gg.hud.Hint("The water is black and doesn't reflect your torch.", 3.0f);
                    } };
    RegisterBelowActions(im);
}

// ---------------------------------------------------------------------------
// Night three: the boiler, the scratching, the bookshelf and the old man
// ---------------------------------------------------------------------------
void BuildCollegeDiscovery(Story::Impl& im, Script& s) {
    Game& gg = im.g;
    Story::Impl* pim = &im;
    s.Until([&gg]() { return gg.Flag("boiler_lit"); });
    s.Wait(2.5f);
    s.Do([&gg]() {
        // the bulb over the hall dies; only the boiler's mouth is left glowing
        Scn().SetLightGroupFlicker("basement", 0.95f);
        audio::Play("switch", 0.4f, 0.6f);
    });
    s.Wait(1.2f);
    s.Do([&gg]() {
        Scn().SetLightGroup("basement", false);
        audio::Play("breaker", 0.5f, 0.6f);
        gg.player.fear = 0.6f;
    });
    s.Wait(3.0f);
    s.Do([&gg]() {
        audio::Play3D("drag_short", CollegeP(4.0f, -2.2f, -7.8f), 0.9f, 1.4f);
        gg.hud.Say("", "(scratching. on the other side of the bookshelf.)", 3.0f);
        gg.SetFlag("scratch_heard");
        gg.hud.Objective("Something is scratching behind the bookshelf.");
    });
    // it keeps scratching until you open it
    s.Run([&gg](float t, float dt) {
        if (fmodf(t, 5.5f) < dt && t > 1.0f) audio::Play3D(fmodf(t, 11.0f) < 5.5f ? "drag_short" : "twig", CollegeP(3.2f, -2.2f, -7.8f), 0.7f, 1.5f);
        return gg.Flag("bookshelf_open");
    });
    s.Do([pim, &gg]() {
        SpawnOldMan(*pim, false);
        gg.hud.Objective("");
    });
    s.Until([&gg]() { return InHiddenRoom(gg.player.feet) || gg.Flag("talk_oldman"); });
    s.Do([&gg]() {
        gg.player.locked = true;
        gg.hud.Letterbox(true);
        if (Actor* a = gg.A("oldman")) { a->lookAt = gg.player.EyePos(); a->lookWeight = 1.0f; }
        gg.actors["oldman"].action = "";
        audio::Play("breath_out", 0.5f, 0.7f);
    });
    s.Say("OLD MAN", "Close it. Close it. He listens through the pipes.")
     .Say("ADAM", "Who are you?")
     .Say("OLD MAN", "I pumped his gas. Forty-one years. Look at my hands. Heavy. I can't lift them past my knees anymore.")
     .Say("OLD MAN", "He's weighing you too. Every shift, a little more of you stays here.")
     .Say("ADAM", "My brother. A man dragged him off the road. I watched him do it.")
     .Say("OLD MAN", "The boy.", 1.6f, 1.2f)
     .Say("OLD MAN", "The boy wasn't screaming, was he?", 3.0f);
    s.Do([pim]() { pim->Choose({ "\"He was terrified.\"", "\"...No. He wasn't.\"" }); });
    s.Until([pim]() { return pim->choiceResult >= 0; });
    SayLines(s, [pim]() -> std::vector<Line> {
        if (pim->choiceResult == 0)
            return { { "OLD MAN", "Terrified people scream, son." }, { "OLD MAN", "You think about that. Not now. Later, when it matters." } };
        return { { "OLD MAN", "No. They never do, the ones who asked for it." } };
    });
    s.Say("OLD MAN", "That door goes down. Under the field, under his tanks. Under everything.")
     .Say("OLD MAN", "The key is with the cows. Third one from the fence. He buried it beside her so she'd keep it.")
     .Say("OLD MAN", "Take these. Fire is the only thing the ground ever gives back.")
     .Do([&gg]() {
         audio::Play("pickup", 0.7f);
         gg.GiveItem("matches");
         gg.hud.Notify("received: a matchbook  (blackwood motor inn)");
     })
     .Say("OLD MAN", "Now go. Before he counts you missing.", 2.6f, 1.4f)
     .Do([&gg]() {
         // the candles gutter and he's simply... finished
         Scn().SetLightGroupFlicker("candles", 0.8f);
         audio::Play("breath_out", 0.6f, 0.5f);
         if (Actor* a = gg.A("oldman")) { a->lookWeight = 0; a->SetPose(PoseSlumped(0)); }
         gg.spirit += 0.25f;
     })
     .Wait(2.0f)
     .Say("ADAM", "...Sir?", 1.8f, 1.5f)
     .Do([&gg]() {
         gg.spirit -= 0.25f;
         Scn().SetLightGroupFlicker("candles", 0.35f);
         gg.SetFlag("oldman_done");
         gg.hud.Letterbox(false);
         gg.hud.FadeTo(1.0f, 0.4f);
         audio::Play("stinger_low", 0.5f);
     })
     .Wait(3.0f)
     .Do([&gg]() { gg.player.locked = false; gg.story->pendingChapter = CH_KEY; });
}

// ---------------------------------------------------------------------------
// Chapter: The Key. Shovel from the barn, dig beside the third cow, run.
// ---------------------------------------------------------------------------
void Story::Impl::BuildKey(Script& s) {
    Game& gg = g;
    s.Do([this, &gg]() {
        gg.story->cutscene = false;
        gg.hud.FadeInstant(1.0f);
        gg.hud.FadeTo(0.0f, 0.4f);
        gg.hud.Title("THE KEY", "third from the fence", 5.0f);
        // out of the college doors, into the fog
        Vector3 p = CollegeP(0.0f, 0.3f, 14.5f);
        gg.player.Spawn(p, CollegeYaw(0, 1) * RAD2DEG);
        gg.player.flashOn = true;
        gg.held = "";
        if (!gg.Item("matches")) gg.GiveItem("matches");
        gg.flags.erase("key_dug");
        audio::Play("door_wood_close", 0.5f);
        checkpoint = p; checkpointYaw = CollegeYaw(0, 1) * RAD2DEG;
    });
    s.Wait(2.0f).Objective("Find the cows. Third from the fence.");
    s.Wait(6.0f).Do([&gg]() { if (gg.held != "shovel") gg.hud.Hint("You'll need something to dig with. There was a barn past the cows.", 5.0f); });
    // digging beside the third cow (hold E with the shovel)
    auto dig = std::make_shared<float>(0.0f);
    s.Run([this, &gg, dig](float t, float dt) {
        Entity* cow = Scn().Find("cow_3");
        if (!cow || gg.held != "shovel") return false;
        Vector3 spot = cow->FocusPoint();
        float d = Vector3Distance(gg.player.feet, spot);
        Vector3 to = Vector3Normalize(Vector3Subtract(spot, gg.player.EyePos()));
        if (d > 2.6f || Vector3DotProduct(to, gg.player.Forward()) < 0.6f) { *dig = Damp(*dig, 0.0f, 3.0f, dt); return false; }
        gg.hud.SetFocus(true);
        gg.hud.SetPrompt("Hold  Dig beside her", *dig);
        bool held = IsKeyDown(KEY_E) || IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
        if (held) {
            *dig += dt / 4.0f;
            if (fmodf(t, 0.8f) < dt) { audio::Play3D("dig", spot, 0.8f, 0.9f + fmodf(t, 1.7f) * 0.1f); gg.player.shake = fmaxf(gg.player.shake, 0.12f); }
        }
        return *dig >= 1.0f;
    });
    s.Do([this, &gg]() {
        audio::Play("metal_hit", 0.5f, 1.4f);
        gg.GiveItem("iron_key");
        gg.SetFlag("key_dug");
        gg.hud.Notify("received: an iron key, wrapped in a strip of hide");
        gg.hud.Say("ADAM", "Got it.", 1.6f);
    });
    s.Wait(1.8f);
    s.Do([this, &gg]() {
        // she hears it leave
        Entity* cow = Scn().Find("cow_3");
        Vector3 p = cow ? cow->base : gg.player.feet;
        float yaw = cow ? cow->worldYaw : 0.0f;
        audio::Play3D("rattle_bones", p, 1.0f, 0.8f);
        audio::Play3D("bone_crack", p, 0.9f);
        gg.ShakeCamera(0.2f);
        gg.player.fear = 1.0f;
        HideEntity(cow);
        AddCreature("cow", 1, p);
        if (Creature* c = FindCreature("cow")) { c->yaw = yaw; c->seed = 3; }
        checkpoint = Vector3Add(p, { -6.0f, 0, 3.0f });
        checkpointYaw = -90.0f;
        onCaught = [this]() {
            if (Creature* c = FindCreature("cow")) { c->pos = c->home; c->state = 0; c->t = 0; c->wake = 0.6f; }
        };
    });
    s.Wait(1.2f).Do([&gg]() { gg.hud.Say("", "(behind you, the sound of something standing up)", 3.0f); });
    s.Wait(1.5f).Objective("Get out of the field.");
    s.Until([this, &gg]() {
        Creature* c = FindCreature("cow");
        return gg.player.feet.x < 41.0f || (c && Vector3Distance(c->pos, gg.player.feet) > 55.0f);
    });
    s.Do([this, &gg]() {
        // she stops at the fence and comes apart where she stands
        if (Creature* c = FindCreature("cow")) {
            audio::Play3D("rattle_bones", c->pos, 1.0f, 0.7f);
            Scn().Spawn("cow_skeleton", { c->pos.x, 0, c->pos.z }, { c->yaw * RAD2DEG, 0, 0 }, { { "variant", "3" }, { "tag", "runtime" } });
            c->state = 4;
        }
        creatures.clear();
        onCaught = nullptr;
        gg.player.fear = 0.6f;
        gg.hud.Hint("She won't cross the fence.", 3.0f);
        if (gg.held == "shovel") gg.held = "";
    });
    s.Wait(3.0f).Objective("The iron door, under the college.");
    s.Until([&gg]() { return gg.Flag("iron_unlocked"); });
    s.Do([&gg]() {
        gg.player.locked = true;
        gg.hud.Letterbox(true);
        audio::Play("breath_out", 0.5f, 0.6f);
        audio::Amb().whispers = 0.35f;
    });
    s.Say("ADAM", "It's warm. The door is warm.", 2.4f, 0.8f);
    s.Do([&gg]() {
        if (Entity* d = Scn().Find("iron_door")) { d->state["open"] = 1.0f; audio::Play3D("iron_door", d->FocusPoint(), 1.0f, 0.8f); }
        gg.ShakeCamera(0.15f);
    });
    s.Wait(2.5f).Do([&gg]() { gg.hud.FadeTo(1.0f, 0.35f); audio::Play("stinger_low", 0.6f); });
    s.Wait(3.2f).Do([&gg]() { audio::Amb().whispers = 0.0f; gg.player.locked = false; gg.story->pendingChapter = CH_BELOW; });
}
