use bevy::{
    ecs::batching::BatchingStrategy,
    prelude::*,
    render::{
        RenderPlugin,
        settings::{RenderCreation, WgpuSettings},
    },
    tasks::available_parallelism,
};

#[derive(Bundle)]
struct ObjectBundle {
    position: Position,
    velocity: Velocity,
    acceleration: Acceleration,
}

#[derive(Component)]
struct Position {
    x: f32,
    y: f32,
    z: f32,
}

#[derive(Component)]
struct Velocity {
    x: f32,
    y: f32,
    z: f32,
}

#[derive(Component)]
struct Acceleration {
    x: f32,
    y: f32,
    z: f32,
}

#[derive(Resource)]
struct ItemCount(usize);

#[derive(Resource)]
struct Iteracoes(u32);

const MAX_ITERACOES: u32 = 1000;
const BATCH_SIZE: usize = 4096;

fn spawn_objects(mut commands: Commands, item_count: Res<ItemCount>) {
    commands.spawn_batch((0..item_count.0).map(|_| ObjectBundle {
        position: Position {
            x: 0.0,
            y: 0.0,
            z: 0.0,
        },
        velocity: Velocity {
            x: 0.0,
            y: 0.0,
            z: 0.0,
        },
        acceleration: Acceleration {
            x: 1.0,
            y: 1.0,
            z: 1.0,
        },
    }));
}

fn apply_gravity(mut query: Query<&mut Acceleration>, time: Res<Time>) {
    let delta = time.delta_secs();
    query
        .par_iter_mut()
        .batching_strategy(BatchingStrategy::fixed(BATCH_SIZE))
        .for_each(|mut acc| {
            acc.y -= 9.85 * delta;
        });
}

fn apply_acceleration(mut query: Query<(&mut Velocity, &Acceleration)>, time: Res<Time>) {
    let delta = time.delta_secs();
    query
        .par_iter_mut()
        .batching_strategy(BatchingStrategy::fixed(BATCH_SIZE))
        .for_each(|(mut vel, acc)| {
            vel.x += acc.x * delta;
            vel.y += acc.y * delta;
            vel.z += acc.z * delta;
        });
}

fn apply_velocity(mut query: Query<(&mut Position, &Velocity)>, time: Res<Time>) {
    let delta = time.delta_secs();
    query
        .par_iter_mut()
        .batching_strategy(BatchingStrategy::fixed(BATCH_SIZE))
        .for_each(|(mut pos, vel)| {
            pos.x += vel.x * delta;
            pos.y += vel.y * delta;
            pos.z += vel.z * delta;
        });
}

fn contar_iteracoes(mut iteracoes: ResMut<Iteracoes>, mut exit: MessageWriter<AppExit>) {
    iteracoes.0 += 1;
    if iteracoes.0 >= MAX_ITERACOES {
        exit.write(AppExit::Success);
    }
}

fn main() {
    let threads = available_parallelism() - 1;

    App::new()
        .add_plugins(
            DefaultPlugins
                .set(RenderPlugin {
                    render_creation: RenderCreation::Automatic(WgpuSettings {
                        backends: None,
                        ..default()
                    }),
                    ..default()
                })
                .set(TaskPoolPlugin {
                    task_pool_options: TaskPoolOptions {
                        min_total_threads: 1,
                        max_total_threads: threads,
                        compute: bevy::app::TaskPoolThreadAssignmentPolicy {
                            min_threads: threads,
                            max_threads: threads,
                            percent: 1.0,
                            on_thread_spawn: None,
                            on_thread_destroy: None,
                        },
                        async_compute: bevy::app::TaskPoolThreadAssignmentPolicy {
                            min_threads: 1,
                            max_threads: 1,
                            percent: 0.0,
                            on_thread_spawn: None,
                            on_thread_destroy: None,
                        },
                        io: bevy::app::TaskPoolThreadAssignmentPolicy {
                            min_threads: 1,
                            max_threads: 1,
                            percent: 0.0,
                            on_thread_spawn: None,
                            on_thread_destroy: None,
                        },
                    },
                }),
        )
        .insert_resource(ItemCount(1_000_000))
        .insert_resource(Iteracoes(0))
        .add_systems(Startup, spawn_objects)
        .add_systems(
            Update,
            (
                contar_iteracoes,
                apply_gravity,
                apply_acceleration,
                apply_velocity,
            ),
        )
        .run();
}
