#include "CannonAimbot.h"
//Potentiol Sources:
// Sea of Cheaters: https://github.com/FreeSoTForALL/Sea-of-Cheaters/blob/main/Cheat/cheat.cpp
// SOT Rust Linux External: (google it)



void CannonAimbot::Run(uintptr_t GNames, uintptr_t LPawn, uintptr_t playerController, std::vector<Entity> ships, std::vector<Entity> Enemies, DrawingContext *ctx, InputManager *inpMngr) {
    uintptr_t cannonActor = GetCannonActor(LPawn, GNames);
    if (cannonActor == 0x0) return;

    float projectileSpeed, projectileGravityScale;
    GetProjectileInfo(cannonActor, GNames, projectileSpeed, projectileGravityScale);
    if (projectileSpeed == 0) { //if on cannon but nothing loaded
        projectileSpeed = this->lastLoadedProjectileSpeed; //Use last loaded projectile speed
        projectileGravityScale = this->lastLoadedProjectileGravityScale;
    }
    //save last loaded projectile speed and gravity scale for next iteration
    this->lastLoadedProjectileSpeed = projectileSpeed;
    this->lastLoadedProjectileGravityScale = projectileGravityScale;

    //Get player camera info
    ptr CameraManager = ReadMemory<ptr>(playerController + Offsets::PlayerCameraManager);
    FCameraCacheEntry CameraCache = ReadMemory<FCameraCacheEntry>(CameraManager + Offsets::CameraCachePrivate);

    //Only needs to be ship velocity as player is stationary
    FVector localPlayerVelocity = GetShipVelocityByDistance(LPawn, ships);

    for (int i = 0; i < ships.size(); i++) {
        Entity ship = ships[i];

        FVector shipLinearVel, shipAngularVel;
        GetShipInfo(ship.pawn, shipLinearVel, shipAngularVel);
    }


}

double get2DDistance(FVector a, FVector b) {
    return std::sqrt(std::pow(a.x - b.x, 2) + std::pow(a.y - b.y, 2));
}

double getLaunchAngle() {

}

double launchAngles() {

}

FVector CannonAimbot::RotationPrediction(FCameraCacheEntry LPCam, FVector LPLinearVel, float ProjectileSpeed, float ProjectileGravityScale, FVector ShipCoords, FVector ShipLinearVel, FVector ShipAngularVel) {
    // Convert to meters
    double gravityM = static_cast<double>(ProjectileGravityScale * 981.0f) / 100.0;
    double projectileSpeedM = static_cast<double>(ProjectileSpeed) / 100.0;

    FVector cannonCoordsM = LPCam.POV.Location / 100.0;
    FVector shipCoordsM = ShipCoords / 100.0;
    FVector LPShipLinearVel = LPLinearVel / 100.0; // Local player ship linear velocity in m/s

    double distance = get2DDistance(cannonCoordsM, shipCoordsM);

    if (distance < 600.0) { //Reasonable max distance
        double linearVelX = ShipLinearVel.x / 100.0;
        double linearVelY = ShipLinearVel.y / 100.0;

        double angularVelZ = ShipAngularVel.z;
        double angularVelRadians = angularVelZ * M_PI / 180.0;
        double speedMagnitude = std::sqrt(linearVelX * linearVelX + linearVelY * linearVelY);

        double diskRadius = speedMagnitude / angularVelRadians;
        double effectiveRadius = diskRadius * 0.98; // 2% margin for error

        double actualHeading = std::atan2(linearVelY, linearVelX);
        double actualHeadingDegrees = actualHeading * 180.0 / M_PI;

        FVector center = {
            static_cast<float>(effectiveRadius * std::cos(actualHeading + M_PI / 2.0) + shipCoordsM.x),
            static_cast<float>(effectiveRadius * std::sin(actualHeading + M_PI / 2.0) + shipCoordsM.y),
            0.0 // Only needs to be 2D
        };

        double angleTheta = actualHeading - M_PI / 2.0;

        double tTime = 0.0;
        double tIterator = 0.1;
        FVector targetPosT;
        while (tTime < 50.0) {
            double wTime = tTime * angularVelRadians; // w = angular velocity * time
            targetPosT.x = (center.x + (effectiveRadius * std::cos(wTime + angleTheta) - LPShipLinearVel.x * tTime));
            targetPosT.y = (center.y + (effectiveRadius * std::sin(wTime + angleTheta) - LPShipLinearVel.y * tTime));

            double nDist = get2DDistance(LPCam.POV.Location, targetPosT);

            double sAngle = getLaunchAngle();
            double sTime = nDist / (projectileSpeedM * std::cos(sAngle));

            if (sTime < tTime) {
                targetPosT.z = launchAngles();
                break;
            }

            tTime += tIterator;
        }

        targetPosT.x *= 100.0;
        targetPosT.y *= 100.0;
        targetPosT.z *= 100.0;

        return targetPosT;
    }
}

uintptr_t CannonAimbot::GetCannonActor(uintptr_t LPawn, uintptr_t GNames) {
    TArray<uintptr_t> LPChildren = ReadMemory<TArray<uintptr_t>>(LPawn + Offsets::Children);
    ptr cannonActor = 0x0;
    for (int i = 0; i < LPChildren.Length(); i++) {
        ptr childActor = ReadMemory<ptr>(LPChildren.GetAddress() + (i * sizeof(ptr)));
        int childID = ReadMemory<int>(childActor + Offsets::ActorID);

        std::string childName = getNameFromPawn(childActor, GNames);

        if (childName.find("IslandCannon") != std::string::npos || childName.find("Cannon_Ship") != std::string::npos) {
            cannonActor = childActor;
            //break; // Real cannons are typically at the bottom of the list so not breaking means we can weed out other actors that are filtered here but are not cannons
        }
    }

    return cannonActor;
}

//TODO: Move logic about using last shot cannonball info into this function
void CannonAimbot::GetProjectileInfo(uintptr_t CannonActor, uintptr_t GNames, float &outProjectileSpeed, float &outProjectileGravityScale) {

    float ProjectileSpeed = ReadMemory<float>(CannonActor + Offsets::ProjectileSpeed);
    float projectileGravityScale = ReadMemory<float>(CannonActor + Offsets::ProjectileGravityScale);

    ptr LoadableComponent = ReadMemory<ptr>(CannonActor + Offsets::LoadableComponent);
    ptr LoadedItem = ReadMemory<ptr>(LoadableComponent + Offsets::LoadableComponentState + Offsets::LoadedItem);

    if (LoadedItem != 0) {
        std::string loadedItemName = getNameFromPawn(LoadedItem, GNames);
        if (loadedItemName.find("Player") != std::string::npos) {
            projectileGravityScale = 1.3f;
        } else if (loadedItemName.find("chain_shot") != std::string::npos) {
            projectileGravityScale = 1.0f;
        }
    }
    outProjectileSpeed = ProjectileSpeed;
    outProjectileGravityScale = projectileGravityScale;
}

void CannonAimbot::GetShipInfo(uintptr_t ShipPawn, FVector &outShipLinearVel, FVector &outShipAngularVel) {
    ptr movementProxyComponent = ReadMemory<ptr>(ShipPawn + Offsets::ShipMovementProxyComponent);
    ptr movementProxyActor = ReadMemory<ptr>(movementProxyComponent + Offsets::ChildActor);
    ptr RepMovement_Address = movementProxyActor + Offsets::ShipMovement + Offsets::ReplicatedShipMovement_Movement; //note: adding, not reading

    outShipLinearVel = ReadMemory<FVector>(RepMovement_Address + Offsets::LinearVelocity);
    outShipAngularVel = ReadMemory<FVector>(RepMovement_Address + Offsets::AngularVelocity);
}

void draw_rotation_prediction(IService& game, const ACannon& projectile_info, glm::dvec3 ship_coords, FRepMovement ship_movement) {
    FRepMovement my_ship_movement = game.get_ship_tracker().value_or(FRepMovement::default_instance()).my_ship_movement;
    LocalPlayer& local_p = game.get_local_player();

    // Scale down for numerical stability, as in the Rust code
    const double gravity = static_cast<double>(projectile_info.gravity_scale * 981.0f) / 100.0;
    const double projectile_speed = static_cast<double>(projectile_info.projectile_speed) / 100.0;

    glm::dvec3 cannon_coords = local_p.camera_pos / 100.0;
    ship_coords /= 100.0;
    my_ship_movement.linear_velocity /= 100.0;
    ship_movement.linear_velocity /= 100.0;

    const double distance = get_2d_distance(cannon_coords, ship_coords);

    if (distance < 600.0) {
        const double angular_velocity_z = ship_movement.angular_velocity.z;
        const double angular_velocity_radians = angular_velocity_z * glm::pi<double>() / 180.0;

        if (std::abs(angular_velocity_radians) < 1e-4) return; // Not turning

        const glm::dvec2 linear_velocity_2d(ship_movement.linear_velocity.x, ship_movement.linear_velocity.y);
        const double speed_magnitude = glm::length(linear_velocity_2d);

        const double disk_radius = speed_magnitude / angular_velocity_radians;
        const double effective_radius = disk_radius * 0.98;

        const double actual_heading = std::atan2(linear_velocity_2d.y, linear_velocity_2d.x);

        // Center of the turning circle
        const glm::dvec2 center = {
            effective_radius * std::cos(actual_heading + glm::pi<double>() / 2.0) + ship_coords.x,
            effective_radius * std::sin(actual_heading + glm::pi<double>() / 2.0) + ship_coords.y
        };

        const double angle_theta = actual_heading - glm::pi<double>() / 2.0;

        glm::dvec3 target_pos_t;
        float t_time = 0.0f;
        const float t_iterator = 0.1f;

        while (t_time < 50.0f) {
            const double w_time = angular_velocity_radians * t_time;

            // Predicted ship position, accounting for our own ship's movement
            target_pos_t.x = center.x + (effective_radius * std::cos(w_time + angle_theta)) - (my_ship_movement.linear_velocity.x * t_time);
            target_pos_t.y = center.y + (effective_radius * std::sin(w_time + angle_theta)) - (my_ship_movement.linear_velocity.y * t_time);
            target_pos_t.z = ship_coords.z;

            const double n_dist = get_2d_distance(cannon_coords, target_pos_t);
            const double angle_tan = get_launch_angle_tan(cannon_coords, target_pos_t, gravity, projectile_speed, n_dist);
            const double s_time = n_dist / (projectile_speed * std::cos(std::atan(angle_tan)));

            if (s_time < t_time) {
                target_pos_t.z = get_aim_z_at_distance(cannon_coords, target_pos_t, gravity, projectile_speed, n_dist);
                break;
            }
            t_time += t_iterator;
        }

        // Scale coordinates back up to world space
        target_pos_t *= 100.0;

        // Draw prediction
        glm::dvec2 screen_pos;
        if (local_p.world_to_screen(target_pos_t, &screen_pos)) {
            Actor my_actor = {screen_pos.x, screen_pos.y, 10.0, 10.0, "", 8, ObjectType::Circle, BLUE};
            std::lock_guard<std::mutex> lock(game.get_draw_list_mutex());
            game.get_draw_list().push_back(my_actor);
        }
    }
}