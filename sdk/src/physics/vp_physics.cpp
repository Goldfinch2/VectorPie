// vp_physics: rigid-body physics (vp_physics.h) on ReactPhysics3D
// (src/third_party/reactphysics3d, zlib license).

#include "../core/vp_internal.h"
#include <vectorpie/vp_physics.h>

#include <reactphysics3d/reactphysics3d.h>

#include <algorithm>
#include <cmath>
#include <new>
#include <vector>

namespace rp = reactphysics3d;

struct vp_body {
    vp_phys          *world = nullptr;
    rp::RigidBody    *body = nullptr;
    rp::Collider     *collider = nullptr;
    rp::CollisionShape *shape = nullptr;
    rp::ConvexMesh   *hull = nullptr;       // the shape's data, if it has its own
    rp::TriangleMesh *mesh = nullptr;
    void             *user = nullptr;
    rp::Vector3       vel_before;           // at the start of the step: contacts' speed
};

struct vp_joint {
    vp_phys   *world = nullptr;
    rp::Joint *joint = nullptr;
    vp_body   *a = nullptr, *b = nullptr;
    rp::JointType type;
};

namespace {

rp::Vector3 rv(vp_vec3 v) { return rp::Vector3(v.x, v.y, v.z); }
vp_vec3 vv(const rp::Vector3 &v) { return vp_v3(v.x, v.y, v.z); }

rp::BodyType bodyType(vp_body_kind k)
{
    return k == VP_BODY_STATIC ? rp::BodyType::STATIC
         : k == VP_BODY_MOVED  ? rp::BodyType::KINEMATIC
                               : rp::BodyType::DYNAMIC;
}

// The contacts that begin, collected as the engine reports them.
struct Listener : public rp::EventListener {
    std::vector<vp_contact> started;

    void onContact(const rp::CollisionCallback::CallbackData &data) override
    {
        for (rp::uint32 i = 0; i < data.getNbContactPairs(); i++) {
            const rp::CollisionCallback::ContactPair pair = data.getContactPair(i);
            if (pair.getEventType() != rp::CollisionCallback::ContactPair::EventType::ContactStart)
                continue;
            if (pair.getNbContactPoints() == 0) continue;
            vp_body *a = (vp_body *)pair.getBody1()->getUserData();
            vp_body *b = (vp_body *)pair.getBody2()->getUserData();
            if (!a || !b) continue;                     // the world's own anchor
            const rp::CollisionCallback::ContactPoint p = pair.getContactPoint(0);
            vp_contact c;
            c.a = a;
            c.b = b;
            c.point = vv(pair.getCollider1()->getLocalToWorldTransform() * p.getLocalPointOnCollider1());
            c.normal = vv(p.getWorldNormal());
            const rp::Vector3 closing = a->vel_before - b->vel_before;
            c.speed = std::max(0.0f, (float)closing.dot(p.getWorldNormal()));
            started.push_back(c);
        }
    }
};

struct Closest : public rp::RaycastCallback {
    bool hit = false;
    vp_hit info{};
    const void *ignore = nullptr;

    rp::decimal notifyRaycastHit(const rp::RaycastInfo &i) override
    {
        if (!i.body || !i.body->getUserData()) return -1;   // the world's anchor: ignore
        if (i.body->getUserData() == ignore) return -1;     // the one asked to pass through
        hit = true;
        info.body = (vp_body *)i.body->getUserData();
        info.point = vv(i.worldPoint);
        info.normal = vv(i.worldNormal);
        info.fraction = (float)i.hitFraction;
        return i.hitFraction;                               // only nearer ones from now on
    }
};

}  // namespace

struct vp_phys {
    rp::PhysicsCommon common;
    rp::PhysicsWorld *world = nullptr;
    Listener listener;
    std::vector<vp_body *> bodies;
    std::vector<vp_joint *> joints;
    rp::RigidBody *anchor = nullptr;        // "the world" for joints with one body
};

// ---------------------------------------------------------------------------
// The world
// ---------------------------------------------------------------------------

int vp_phys_create(vp_phys **out)
{
    if (!out) return vp_fail(VP_ERR_INVALID, "vp_phys_create: no out");
    vp_phys *w = new (std::nothrow) vp_phys();
    if (!w) return vp_fail(VP_ERR_NOMEM, "vp_phys_create: out of memory");
    rp::PhysicsWorld::WorldSettings settings;
    settings.gravity = rp::Vector3(0, -9.81f, 0);
    w->world = w->common.createPhysicsWorld(settings);
    if (!w->world) {
        delete w;
        return vp_fail(VP_ERR_NOMEM, "vp_phys_create: no world");
    }
    w->world->setEventListener(&w->listener);
    *out = w;
    return VP_OK;
}

void vp_phys_free(vp_phys *w)
{
    if (!w) return;
    for (vp_joint *j : w->joints) delete j;
    for (vp_body *b : w->bodies) delete b;
    w->common.destroyPhysicsWorld(w->world);    // the bodies, joints and colliders in it
    delete w;                                   // PhysicsCommon frees the shapes and meshes
}

void vp_phys_set_gravity(vp_phys *w, vp_vec3 g)
{
    if (w) w->world->setGravity(rv(g));
}

void vp_phys_set_iterations(vp_phys *w, int velocity, int position)
{
    if (!w) return;
    if (velocity > 0) w->world->setNbIterationsVelocitySolver((rp::uint16)velocity);
    if (position > 0) w->world->setNbIterationsPositionSolver((rp::uint32)position);
}

void vp_phys_step(vp_phys *w, float dt)
{
    if (!w || dt <= 0) return;
    for (vp_body *b : w->bodies) b->vel_before = b->body->getLinearVelocity();
    w->listener.started.clear();
    w->world->update(dt);
}

// ---------------------------------------------------------------------------
// Bodies
// ---------------------------------------------------------------------------

namespace {

vp_body *addBody(vp_phys *w, vp_body_kind kind, vp_vec3 pos, rp::CollisionShape *shape, float mass)
{
    vp_body *b = new (std::nothrow) vp_body();
    if (!b) {
        vp_fail(VP_ERR_NOMEM, "vp_phys: out of memory");
        return nullptr;
    }
    b->world = w;
    b->shape = shape;
    b->body = w->world->createRigidBody(rp::Transform(rv(pos), rp::Quaternion::identity()));
    b->body->setUserData(b);
    b->collider = b->body->addCollider(shape, rp::Transform::identity());
    rp::Material &m = b->collider->getMaterial();
    m.setBounciness(0.1f);
    m.setFrictionCoefficient(0.5f);
    b->body->setType(bodyType(kind));
    b->body->setAngularDamping(0.05f);
    if (kind == VP_BODY_DYNAMIC) vp_body_set_mass(b, mass);
    w->bodies.push_back(b);
    return b;
}

}  // namespace

vp_body *vp_phys_add_box(vp_phys *w, vp_body_kind kind, vp_vec3 pos, vp_vec3 half, float mass)
{
    if (!w || half.x <= 0 || half.y <= 0 || half.z <= 0) {
        vp_fail(VP_ERR_INVALID, "vp_phys_add_box: a box needs half sizes above 0");
        return nullptr;
    }
    return addBody(w, kind, pos, w->common.createBoxShape(rv(half)), mass);
}

vp_body *vp_phys_add_sphere(vp_phys *w, vp_body_kind kind, vp_vec3 pos, float radius, float mass)
{
    if (!w || radius <= 0) {
        vp_fail(VP_ERR_INVALID, "vp_phys_add_sphere: radius must be above 0");
        return nullptr;
    }
    return addBody(w, kind, pos, w->common.createSphereShape(radius), mass);
}

vp_body *vp_phys_add_capsule(vp_phys *w, vp_body_kind kind, vp_vec3 pos, float radius,
                             float height, float mass)
{
    if (!w || radius <= 0 || height <= 0) {
        vp_fail(VP_ERR_INVALID, "vp_phys_add_capsule: radius and height must be above 0");
        return nullptr;
    }
    return addBody(w, kind, pos, w->common.createCapsuleShape(radius, height), mass);
}

vp_body *vp_phys_add_hull(vp_phys *w, vp_body_kind kind, vp_vec3 pos, const vp_vec3 *points,
                          int count, float mass)
{
    if (!w || !points || count < 4) {
        vp_fail(VP_ERR_INVALID, "vp_phys_add_hull: needs 4 points or more");
        return nullptr;
    }
    rp::VertexArray va(points, sizeof(vp_vec3), (rp::uint32)count, rp::VertexArray::DataType::VERTEX_FLOAT_TYPE);
    std::vector<rp::Message> messages;
    rp::ConvexMesh *hull = w->common.createConvexMesh(va, messages);
    if (!hull) {
        vp_fail(VP_ERR_INVALID, "vp_phys_add_hull: %s",
                messages.empty() ? "no hull from these points" : messages[0].text.c_str());
        return nullptr;
    }
    vp_body *b = addBody(w, kind, pos, w->common.createConvexMeshShape(hull), mass);
    if (b) b->hull = hull;
    return b;
}

vp_body *vp_phys_add_triangles(vp_phys *w, vp_body_kind kind, vp_vec3 pos,
                               const vp_vec3 *points, int npoints, const int *tris, int ntris)
{
    if (!w || !points || !tris || npoints < 3 || ntris < 1) {
        vp_fail(VP_ERR_INVALID, "vp_phys_add_triangles: needs points and triangles");
        return nullptr;
    }
    if (kind == VP_BODY_DYNAMIC) {
        vp_fail(VP_ERR_INVALID, "vp_phys_add_triangles: scenery only (static or moved), not dynamic");
        return nullptr;
    }
    for (int i = 0; i < ntris * 3; i++)
        if (tris[i] < 0 || tris[i] >= npoints) {
            vp_fail(VP_ERR_INVALID, "vp_phys_add_triangles: index %d out of range", tris[i]);
            return nullptr;
        }
    rp::TriangleVertexArray tva((rp::uint32)npoints, points, sizeof(vp_vec3), (rp::uint32)ntris, tris,
                                3 * sizeof(int), rp::TriangleVertexArray::VertexDataType::VERTEX_FLOAT_TYPE,
                                rp::TriangleVertexArray::IndexDataType::INDEX_INTEGER_TYPE);
    std::vector<rp::Message> messages;
    rp::TriangleMesh *mesh = w->common.createTriangleMesh(tva, messages);
    if (!mesh) {
        vp_fail(VP_ERR_INVALID, "vp_phys_add_triangles: %s",
                messages.empty() ? "no mesh from these triangles" : messages[0].text.c_str());
        return nullptr;
    }
    vp_body *b = addBody(w, kind, pos, w->common.createConcaveMeshShape(mesh), 0);
    if (b) b->mesh = mesh;
    return b;
}

void vp_body_remove(vp_body *b)
{
    if (!b) return;
    vp_phys *w = b->world;
    for (size_t i = 0; i < w->joints.size();) {
        vp_joint *j = w->joints[i];
        if (j->a == b || j->b == b) vp_joint_remove(j);
        else i++;
    }
    w->world->destroyRigidBody(b->body);
    w->bodies.erase(std::remove(w->bodies.begin(), w->bodies.end(), b), w->bodies.end());
    delete b;
}

// ---------------------------------------------------------------------------
// Reading and moving
// ---------------------------------------------------------------------------

vp_xf vp_body_place(const vp_body *b)
{
    if (!b) return vp_xf_identity();
    const rp::Transform &t = b->body->getTransform();
    const rp::Matrix3x3 m = t.getOrientation().getMatrix();
    vp_xf x;
    for (int r = 0; r < 3; r++)
        for (int c = 0; c < 3; c++) x.m[r * 3 + c] = m[r][c];
    x.t[0] = t.getPosition().x;
    x.t[1] = t.getPosition().y;
    x.t[2] = t.getPosition().z;
    return x;
}

vp_vec3 vp_body_pos(const vp_body *b) { return b ? vv(b->body->getTransform().getPosition()) : vp_v3(0, 0, 0); }
vp_vec3 vp_body_vel(const vp_body *b) { return b ? vv(b->body->getLinearVelocity()) : vp_v3(0, 0, 0); }
vp_vec3 vp_body_spin(const vp_body *b) { return b ? vv(b->body->getAngularVelocity()) : vp_v3(0, 0, 0); }
int vp_body_sleeping(const vp_body *b) { return b && b->body->isSleeping(); }

vp_body_kind vp_body_get_kind(const vp_body *b)
{
    if (!b) return VP_BODY_STATIC;
    switch (b->body->getType()) {
    case rp::BodyType::STATIC:    return VP_BODY_STATIC;
    case rp::BodyType::KINEMATIC: return VP_BODY_MOVED;
    default:                      return VP_BODY_DYNAMIC;
    }
}

void vp_body_set_place(vp_body *b, const vp_xf *place)
{
    if (!b || !place) return;
    rp::Matrix3x3 m(place->m[0], place->m[1], place->m[2],
                    place->m[3], place->m[4], place->m[5],
                    place->m[6], place->m[7], place->m[8]);
    rp::Quaternion q(m);
    q.normalize();
    b->body->setTransform(rp::Transform(rp::Vector3(place->t[0], place->t[1], place->t[2]), q));
}

void vp_body_set_pos(vp_body *b, vp_vec3 pos)
{
    if (!b) return;
    rp::Transform t = b->body->getTransform();
    t.setPosition(rv(pos));
    b->body->setTransform(t);
}

void vp_body_set_vel(vp_body *b, vp_vec3 v) { if (b) b->body->setLinearVelocity(rv(v)); }
void vp_body_set_spin(vp_body *b, vp_vec3 s) { if (b) b->body->setAngularVelocity(rv(s)); }

void vp_body_impulse(vp_body *b, vp_vec3 impulse)
{
    if (!b || b->body->getType() != rp::BodyType::DYNAMIC) return;
    const float mass = b->body->getMass();
    if (mass <= 0) return;
    b->body->setLinearVelocity(b->body->getLinearVelocity() + rv(impulse) / mass);
    b->body->setIsSleeping(false);
}

void vp_body_impulse_at(vp_body *b, vp_vec3 impulse, vp_vec3 point)
{
    if (!b || b->body->getType() != rp::BodyType::DYNAMIC) return;
    vp_body_impulse(b, impulse);
    // the turn: the inertia in the body's own axes (where it is diagonal)
    const rp::Transform &t = b->body->getTransform();
    const rp::Vector3 centre = t * b->body->getLocalCenterOfMass();
    const rp::Vector3 torque = (rv(point) - centre).cross(rv(impulse));
    const rp::Quaternion &q = t.getOrientation();
    const rp::Vector3 local = q.getInverse() * torque;
    const rp::Vector3 &inertia = b->body->getLocalInertiaTensor();
    rp::Vector3 dw(inertia.x > 0 ? local.x / inertia.x : 0,
                   inertia.y > 0 ? local.y / inertia.y : 0,
                   inertia.z > 0 ? local.z / inertia.z : 0);
    b->body->setAngularVelocity(b->body->getAngularVelocity() + q * dw);
}

void vp_body_force(vp_body *b, vp_vec3 force)
{
    if (b) b->body->applyWorldForceAtCenterOfMass(rv(force));
}

void vp_body_set_kind(vp_body *b, vp_body_kind kind)
{
    if (b) b->body->setType(bodyType(kind));
}

void vp_body_set_mass(vp_body *b, float mass)
{
    if (!b || mass <= 0) return;
    // through the density: the mass, its middle and its inertia all follow
    const float volume = (float)b->shape->getVolume();
    if (volume > 0) b->collider->getMaterial().setMassDensity(mass / volume);
    b->body->updateMassPropertiesFromColliders();
}

void vp_body_set_material(vp_body *b, float bounce, float friction)
{
    if (!b) return;
    rp::Material &m = b->collider->getMaterial();
    m.setBounciness(std::min(1.0f, std::max(0.0f, bounce)));
    m.setFrictionCoefficient(std::max(0.0f, friction));
}

void vp_body_set_damping(vp_body *b, float linear, float angular)
{
    if (!b) return;
    b->body->setLinearDamping(std::max(0.0f, linear));
    b->body->setAngularDamping(std::max(0.0f, angular));
}

void vp_body_lock_rotation(vp_body *b, int x, int y, int z)
{
    if (b) b->body->setAngularLockAxisFactor(rp::Vector3(x ? 1.0f : 0.0f, y ? 1.0f : 0.0f, z ? 1.0f : 0.0f));
}

void vp_body_set_gravity(vp_body *b, int on) { if (b) b->body->enableGravity(on != 0); }
void vp_body_set_sleep(vp_body *b, int allowed) { if (b) b->body->setIsAllowedToSleep(allowed != 0); }
void vp_body_wake(vp_body *b) { if (b) b->body->setIsSleeping(false); }
void vp_body_set_user(vp_body *b, void *user) { if (b) b->user = user; }
void *vp_body_user(const vp_body *b) { return b ? b->user : nullptr; }

// ---------------------------------------------------------------------------
// Joints
// ---------------------------------------------------------------------------

namespace {

rp::RigidBody *partner(vp_phys *w, vp_body *b)
{
    if (b) return b->body;
    if (!w->anchor) {                       // "the world": a static body, never drawn
        w->anchor = w->world->createRigidBody(rp::Transform::identity());
        w->anchor->setType(rp::BodyType::STATIC);
    }
    return w->anchor;
}

vp_joint *addJoint(vp_phys *w, vp_body *a, vp_body *b, const rp::JointInfo &info)
{
    rp::Joint *j = w->world->createJoint(info);
    if (!j) {
        vp_fail(VP_ERR_INVALID, "vp_phys: the joint could not be made");
        return nullptr;
    }
    vp_joint *out = new (std::nothrow) vp_joint();
    if (!out) {
        w->world->destroyJoint(j);
        vp_fail(VP_ERR_NOMEM, "vp_phys: out of memory");
        return nullptr;
    }
    out->world = w;
    out->joint = j;
    out->a = a;
    out->b = b;
    out->type = info.type;
    w->joints.push_back(out);
    return out;
}

}  // namespace

vp_joint *vp_phys_ball_joint(vp_phys *w, vp_body *a, vp_body *b, vp_vec3 anchor)
{
    if (!w || !a) return nullptr;
    rp::BallAndSocketJointInfo info(a->body, partner(w, b), rv(anchor));
    info.isCollisionEnabled = false;        // joined bodies pass through each other
    return addJoint(w, a, b, info);
}

vp_joint *vp_phys_hinge(vp_phys *w, vp_body *a, vp_body *b, vp_vec3 anchor, vp_vec3 axis)
{
    if (!w || !a) return nullptr;
    rp::HingeJointInfo info(a->body, partner(w, b), rv(anchor), rv(vp_v3_normalize(axis)));
    info.isCollisionEnabled = false;
    return addJoint(w, a, b, info);
}

vp_joint *vp_phys_slider(vp_phys *w, vp_body *a, vp_body *b, vp_vec3 anchor, vp_vec3 axis)
{
    if (!w || !a) return nullptr;
    rp::SliderJointInfo info(a->body, partner(w, b), rv(anchor), rv(vp_v3_normalize(axis)));
    info.isCollisionEnabled = false;
    return addJoint(w, a, b, info);
}

vp_joint *vp_phys_fixed_joint(vp_phys *w, vp_body *a, vp_body *b, vp_vec3 anchor)
{
    if (!w || !a) return nullptr;
    rp::FixedJointInfo info(a->body, partner(w, b), rv(anchor));
    info.isCollisionEnabled = false;
    return addJoint(w, a, b, info);
}

void vp_joint_remove(vp_joint *j)
{
    if (!j) return;
    vp_phys *w = j->world;
    w->world->destroyJoint(j->joint);
    w->joints.erase(std::remove(w->joints.begin(), w->joints.end(), j), w->joints.end());
    delete j;
}

// The engine measures hinges and sliders from body 2's side: its positions
// and limits are negated here, so they count the way `a` (body 1) moves, as
// the motor does.
void vp_joint_limits(vp_joint *j, int on, float min, float max)
{
    if (!j) return;
    if (min > max) { float t = min; min = max; max = t; }
    if (j->type == rp::JointType::HINGEJOINT) {
        rp::HingeJoint *h = static_cast<rp::HingeJoint *>(j->joint);
        h->setMinAngleLimit(-max);
        h->setMaxAngleLimit(-min);
        h->enableLimit(on != 0);
    } else if (j->type == rp::JointType::SLIDERJOINT) {
        rp::SliderJoint *s = static_cast<rp::SliderJoint *>(j->joint);
        s->setMinTranslationLimit(-max);
        s->setMaxTranslationLimit(-min);
        s->enableLimit(on != 0);
    }
}

// The engine drives (body 1 - body 2) to minus its motor speed: negated,
// `a` (body 1) turns or slides at `speed` along the axis.
void vp_joint_motor(vp_joint *j, float speed, float strength)
{
    if (!j) return;
    if (j->type == rp::JointType::HINGEJOINT) {
        rp::HingeJoint *h = static_cast<rp::HingeJoint *>(j->joint);
        h->setMotorSpeed(-speed);
        h->setMaxMotorTorque(std::max(0.0f, strength));
        h->enableMotor(strength > 0);
    } else if (j->type == rp::JointType::SLIDERJOINT) {
        rp::SliderJoint *s = static_cast<rp::SliderJoint *>(j->joint);
        s->setMotorSpeed(-speed);
        s->setMaxMotorForce(std::max(0.0f, strength));
        s->enableMotor(strength > 0);
    }
}

float vp_joint_position(const vp_joint *j)
{
    if (!j) return 0;
    if (j->type == rp::JointType::HINGEJOINT) return -static_cast<rp::HingeJoint *>(j->joint)->getAngle();
    if (j->type == rp::JointType::SLIDERJOINT) return -static_cast<rp::SliderJoint *>(j->joint)->getTranslation();
    return 0;
}

// ---------------------------------------------------------------------------
// Rays and contacts
// ---------------------------------------------------------------------------

int vp_phys_raycast(vp_phys *w, vp_vec3 from, vp_vec3 to, vp_hit *hit)
{
    return vp_phys_raycast_ignore(w, from, to, nullptr, hit);
}

int vp_phys_raycast_ignore(vp_phys *w, vp_vec3 from, vp_vec3 to, const vp_body *ignore, vp_hit *hit)
{
    if (!w) return 0;
    Closest c;
    c.ignore = ignore;
    w->world->raycast(rp::Ray(rv(from), rv(to)), &c);
    if (!c.hit) return 0;
    if (hit) *hit = c.info;
    return 1;
}

int vp_phys_contact_count(const vp_phys *w)
{
    return w ? (int)w->listener.started.size() : 0;
}

int vp_phys_contact(const vp_phys *w, int i, vp_contact *out)
{
    if (!w || i < 0 || i >= (int)w->listener.started.size() || !out) return 0;
    *out = w->listener.started[(size_t)i];
    return 1;
}
