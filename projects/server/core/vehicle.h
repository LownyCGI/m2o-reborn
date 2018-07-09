// =======================================================================//
// !
// ! General stuff
// !
// =======================================================================//

void on_car_create_command(librg_message_t *msg) {
    auto player     = librg_entity_find(msg->ctx, msg->peer);
    auto vehicleid  = m2o_vehicle_create();

    m2o_vehicle_position_set(vehicleid, vec3(
        player->position.x + 3.0f,
        player->position.y,
        player->position.z + 0.05f
    ));

    // log
    print_posm(m2o_vehicle_position_get(vehicleid), "created a vehicle at: ");
}

void on_car_create(librg_event_t *event) {
    auto car = get_car(event->entity);

    librg_data_wu16(event->data, car->model);
    librg_data_wu8(event->data, car->state);
    librg_data_wptr(event->data, &car->stream, sizeof(car->stream));
}

void on_car_remove(librg_event_t *event) {

}


// =======================================================================//
// !
// ! Vehicle events
// !
// =======================================================================//

void on_car_enter_start(librg_message_t *msg) {
    auto player  = librg_entity_find(msg->ctx, msg->peer);
    auto vehicle = librg_entity_fetch(msg->ctx, librg_data_ru32(msg->data));
    mod_assert_msg(vehicle && player, "trying to enter invalid vehicle");

    auto seat = librg_data_ru8(msg->data);
    auto ped = get_ped(player);

    ped->state = PED_IN_CAR;
    ped->vehicle = vehicle;
    ped->seat = seat;

    if (seat == 1) {
        librg_entity_control_set(msg->ctx, vehicle->id, player->client_peer);
    }

    mod_log("[info] ped: %u becomes member of: %u on seat: %u\n", player->id, vehicle->id, seat);
    mod_message_send_instream_except(msg->ctx, MOD_CAR_ENTER_START, player->id, player->client_peer, [&](librg_data_t *data) {
        librg_data_wu32(data, player->id);
        librg_data_wu32(data, vehicle->id);
        librg_data_wu8(data, seat);
    });

    m2o_args args = {0};
    m2o_args_init(&args);
    m2o_args_push_integer(&args, player->id);
    m2o_args_push_integer(&args, vehicle->id);
    m2o_args_push_integer(&args, seat);
    m2o_event_trigger("player_vehicle_enter", &args);
    m2o_args_free(&args);
}

void on_car_enter_finish(librg_message_t *msg) {
    auto player = librg_entity_find(msg->ctx, msg->peer);

    mod_message_send_instream_except(msg->ctx, MOD_CAR_ENTER_FINISH, player->id, player->client_peer, [&](librg_data_t *data) {
        librg_data_wu32(data, player->id);
    });
}

void on_car_exit_start(librg_message_t *msg) {
    auto player = librg_entity_find(msg->ctx, msg->peer);
    auto ped = get_ped(player);
    mod_log("player: %d is trying to leave his current car\n", player->id);

    // TODO: does he still need to control and stream that car? most probably yea
    // librg_entity_control_remove(msg->ctx, player->vehicle->id);

    mod_message_send_instream_except(msg->ctx, MOD_CAR_EXIT_START, player->id, player->client_peer, [&](librg_data_t *data) {
        librg_data_wu32(data, player->id);
    });

    m2o_args args = {0};
    m2o_args_init(&args);
    m2o_args_push_integer(&args, player->id);
    m2o_args_push_integer(&args, ped->vehicle->id);
    m2o_args_push_integer(&args, ped->seat);
    m2o_event_trigger("player_vehicle_exit", &args);
    m2o_args_free(&args);
}

void on_car_exit_finish(librg_message_t *msg) {
    auto player  = librg_entity_find(msg->ctx, msg->peer);
    auto ped = get_ped(player);

    ped->state = PED_ON_GROUND;
    ped->vehicle = nullptr;
    ped->seat = 0;

    mod_message_send_instream_except(msg->ctx, MOD_CAR_EXIT_FINISH, player->id, player->client_peer, [&](librg_data_t *data) {
        librg_data_wu32(data, player->id);
    });
}