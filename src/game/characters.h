// Sculpted (SDF) humanoid characters with a simple bone rig and procedural animation.
#pragma once
#include "engine/renderer.h"
#include "engine/materials.h"

enum Bone {
    B_PELVIS, B_SPINE, B_CHEST, B_NECK, B_HEAD,
    B_LUARM, B_LFARM, B_LHAND, B_RUARM, B_RFARM, B_RHAND,
    B_LTHIGH, B_LSHIN, B_LFOOT, B_RTHIGH, B_RSHIN, B_RFOOT,
    B_COUNT
};

enum HeadType { HEAD_HUMAN, HEAD_FACELESS, HEAD_PIG, HEAD_GOAT, HEAD_DOG, HEAD_HOLLOW };
enum TopStyle { TOP_BARE, TOP_TSHIRT, TOP_HOODIE, TOP_JACKET, TOP_SHIRT_APRON, TOP_RAGS, TOP_OVERALLS };
enum HairStyle { HAIR_BALD, HAIR_SHORT, HAIR_MESSY, HAIR_WISPS, HAIR_LONG };

struct BodySpec {
    float height = 1.75f;
    float thin = 0.3f;          // 0 stocky .. 1 skeletal
    float limbLen = 1.0f;       // limb length multiplier (elongated horrors > 1)
    float headScale = 1.0f;
    float neckLen = 1.0f;
    float shoulders = 1.0f;
    float fingerLen = 1.0f;
    float eyeSink = 0.0f;       // deeper sockets
    float jawDrop = 0.0f;       // open mouth
    float noseLen = 1.0f;
    float smile = 0.0f;         // mouth corner stretch
    bool ribs = false;
    bool beard = false;
    HeadType head = HEAD_HUMAN;
    TopStyle top = TOP_TSHIRT;
    HairStyle hair = HAIR_SHORT;
    int skinMat = MAT_SKIN, topMat = MAT_CLOTH_GREY, pantsMat = MAT_DENIM, shoeMat = MAT_LEATHER, hairMat = MAT_HAIR;
    int accentMat = MAT_CLOTH_BROWN;   // apron / overall bib
    bool barefoot = false;
    bool pants = true;
    uint32_t seed = 1;
};

struct Pose {
    Vector3 rot[B_COUNT]{};
    Vector3 root{};             // pelvis offset from rest
    float rootYaw = 0;
};
Pose PoseLerp(const Pose& a, const Pose& b, float t);

// Library poses/animations (t = seconds or phase as noted)
Pose PoseStand(float t, float breath = 1.0f);
Pose PoseWalk(float phase, float amount, int style = 0);   // style 0 normal, 1 stalk, 2 limp, 3 hunched
Pose PoseRun(float phase);
Pose PoseCrawl(float phase);
Pose PoseSitDrive(float t, float steer);
Pose PoseSitPassenger(float t, float laugh);
Pose PoseSlumped(float t);
Pose PoseLyingBack(float t);
Pose PoseDragged(float t);
Pose PoseDragging(float phase);
Pose PoseTiedChair(float t, float struggle);
Pose PoseCounterLean(float t);
Pose PoseReach(float t);
Pose PoseCry(float t);
Pose PoseCoverOver(float t);
Pose PoseKneel(float t);
Pose PoseRide(float t);            // sitting astride (animal riders / cart drivers)
Pose PoseHarnessed(float phase);   // human pulling a cart on all fours

struct CharModel {
    BodySpec spec;
    Vector3 offset[B_COUNT]{};
    struct Part { int bone; const MeshAsset* mesh; int mat; };
    std::vector<Part> parts;
    float hipHeight = 0.95f;
};

CharModel* BuildCharacter(const BodySpec& spec);

class Actor {
public:
    std::string name;
    CharModel* model = nullptr;
    Vector3 pos{};
    float yaw = 0;               // radians
    Pose pose;                   // current (blended) pose
    Pose target;                 // pose being blended towards
    float blendSpeed = 6.0f;
    bool visible = true;
    Color tint = WHITE;
    Vector3 lookAt{};
    float lookWeight = 0.0f;
    float scale = 1.0f;
    float pitch = 0, roll = 0;   // whole-body tilt (lying, falling)

    void SetPose(const Pose& p, bool snap = false) { target = p; if (snap) pose = p; }
    void Update(float dt);
    void Draw() const;
    void BoneMatrices(Matrix out[B_COUNT]) const;
    Vector3 BonePos(int bone, Vector3 local = { 0, 0, 0 }) const;
    Vector3 HeadPos() const { return BonePos(B_HEAD, { 0, 0.1f, 0.05f }); }
    Matrix RootXf() const;
};

// Preset specs for the cast
BodySpec SpecAdam();
BodySpec SpecZain();
BodySpec SpecGrethnar();
BodySpec SpecDragger();
BodySpec SpecOldMan();
BodySpec SpecCustomer(uint32_t seed);
BodySpec SpecCrawler(uint32_t seed);
BodySpec SpecPigMan(uint32_t seed);
BodySpec SpecGoatMan(uint32_t seed);
BodySpec SpecMother();
