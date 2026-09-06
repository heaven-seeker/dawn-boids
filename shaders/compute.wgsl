struct SimulationParams {
    deltaTime: f32,
    visualRange: f32,
    protectedRange: f32,
    strangerProtectedRange: f32,
    maxSpeed: f32,
    minSpeed: f32,
    cubeSize: f32,
    cohesionFactor: f32,
    alignmentFactor: f32,
    separationFactor: f32,
    turnFactor: f32,
    strangeForceFactor: f32,
    visionRadius: f32,
    margin: f32,
    activeBoidsCount: u32,
    divideFlocks: u32,

    gridWidth: u32,
    gridHeight: u32,
    gridDepth: u32,
    cellSize: f32,
}

struct BoidData {
    position: vec4<f32>,
    velocity: vec4<f32>,
    centerOfMass: vec4<f32>,
};

@group(0) @binding(0) var<uniform> params : SimulationParams;
@group(0) @binding(1) var<storage, read> boidsIn: array<BoidData>;
@group(0) @binding(2) var<storage, read_write> boidsOut: array<BoidData>;
@group(0) @binding(3) var<storage, read_write> gridCounters: array<atomic<u32>>;
@group(0) @binding(4) var<storage, read_write> gridCells: array<u32>;

const MAX_BOIDS_PER_CELL: u32 = 32u;
const MAX_CELLS: u32 = 512000u;

@compute @workgroup_size(64)
fn compute_main(@builtin(global_invocation_id) GlobalInvocationID : vec3<u32>) {
    let index = GlobalInvocationID.x;
    
    if (index >= params.activeBoidsCount) {
        return;
    }

    var currentBoid = boidsIn[index];
    let flockId = currentBoid.position.w;
    let boidCount = arrayLength(&boidsIn);

    var numNeighbors = 0u;
    var centerOfMass = vec4<f32>(0.0);
    var avgVelocity = vec4<f32>(0.0);
    var separationVector = vec4<f32>(0.0);

    let boidDir = normalize(currentBoid.velocity.xyz);

    let cell_x = u32((currentBoid.position.x + params.cubeSize) / params.cellSize);
    let cell_y = u32((currentBoid.position.y + params.cubeSize) / params.cellSize);
    let cell_z = u32((currentBoid.position.z + params.cubeSize) / params.cellSize);

    let start_x = max(0, i32(cell_x) - 1);
    let end_x   = min(i32(params.gridWidth) - 1, i32(cell_x) + 1);

    let start_y = max(0, i32(cell_y) - 1);
    let end_y   = min(i32(params.gridHeight) - 1, i32(cell_y) + 1);

    let start_z = max(0, i32(cell_z) - 1);
    let end_z = min(i32(params.gridDepth) - 1, i32(cell_z) + 1);

    for (var i = start_z; i <= end_z; i++) {
        for (var j = start_y; j <= end_y; j++) {
            for (var k = start_x; k <= end_x; k++) {
                let cell_hash = (u32(i) * params.gridHeight + u32(j)) * params.gridWidth + u32(k);
                let boidCounter: u32 = atomicLoad(&gridCounters[cell_hash]);

                for (var c = 0u; c < boidCounter; c++) {
                    let neighbour_index = gridCells[cell_hash * MAX_BOIDS_PER_CELL + c];
                    let neighbour = boidsIn[neighbour_index];

                    if (index == neighbour_index) {continue;}

                    let diff = neighbour.position.xyz - currentBoid.position.xyz;
                    let dist = length(diff);
                    if (dist < 0.0001) { continue; }
                    
                    let toNeighborDir = diff / dist;

                    if (dot(boidDir, toNeighborDir) > params.visionRadius) {
                        if (dist <= params.visualRange) {
                            let isFriendly = (params.divideFlocks == 0u) || (neighbour.position.w == flockId);
                            if (isFriendly) {
                                numNeighbors += 1u;
                                centerOfMass += neighbour.position;
                                avgVelocity += neighbour.velocity;

                                if (dist <= params.protectedRange) {
                                    let force = -diff;
                                    separationVector += vec4<f32>(force / (dist * dist), 0.0);
                                }
                            } else {
                                if (dist <= params.strangerProtectedRange) {
                                    let force = -diff * params.strangeForceFactor; // new param
                                    separationVector += vec4<f32>(force / (dist * dist), 0.0);
                                    
                                }
                            }
                            
                        }
                    }
                }
            }
        }
    }

    if (numNeighbors > 0u) {
        let nFloat = f32(numNeighbors);
        centerOfMass /= nFloat;
        avgVelocity /= nFloat;

        let desiredCohesion = normalize(centerOfMass.xyz - currentBoid.position.xyz) * params.maxSpeed;
        let steerCohesion = desiredCohesion - currentBoid.velocity.xyz;
        currentBoid.velocity += vec4<f32>(steerCohesion * params.cohesionFactor, 0.0);

        let oldCoM = currentBoid.centerOfMass.xyz;
        let smoothCoM = mix(oldCoM, centerOfMass.xyz, 0.1);
        currentBoid.centerOfMass = vec4<f32>(smoothCoM, 1.0);

        let desiredAlignment = normalize(avgVelocity.xyz) * params.maxSpeed;
        let steerAlignment = desiredAlignment - currentBoid.velocity.xyz;
        currentBoid.velocity += vec4<f32>(steerAlignment * params.alignmentFactor, 0.0);
    } else {
        currentBoid.centerOfMass = currentBoid.position;
    }

    currentBoid.velocity += separationVector * params.separationFactor;

    var velocityLength = length(currentBoid.velocity.xyz);
    let dir = normalize(currentBoid.velocity.xyz);
    if (velocityLength > params.maxSpeed) {
        currentBoid.velocity = vec4<f32>(dir * params.maxSpeed, 0.0);
    } else if (velocityLength < params.minSpeed) {
        currentBoid.velocity = vec4<f32>(dir * params.minSpeed, 0.0);
    }

    if (currentBoid.position.x < -params.cubeSize + params.margin) {
        currentBoid.velocity.x += params.turnFactor;
    } else if (currentBoid.position.x > params.cubeSize - params.margin) {
        currentBoid.velocity.x -= params.turnFactor;
    }

    if (currentBoid.position.y < -params.cubeSize + params.margin) {
        currentBoid.velocity.y += params.turnFactor;
    } else if (currentBoid.position.y > params.cubeSize - params.margin) {
        currentBoid.velocity.y -= params.turnFactor;
    }

    if (currentBoid.position.z < -params.cubeSize + params.margin) {
        currentBoid.velocity.z += params.turnFactor;
    } else if (currentBoid.position.z > params.cubeSize - params.margin) {
        currentBoid.velocity.z -= params.turnFactor;
    }

    currentBoid.position += vec4<f32>(currentBoid.velocity.xyz * params.deltaTime, 0.0);
    boidsOut[index] = currentBoid;
}

@compute @workgroup_size(64)
fn build_grid(@builtin(global_invocation_id) global_id: vec3<u32>) {
    let boid_index = global_id.x;

    if (boid_index >= arrayLength(&boidsIn)) {
        return;
    }

    let pos = boidsIn[boid_index].position;

    let cell_x = u32((pos.x + params.cubeSize) / params.cellSize);
    let cell_y = u32((pos.y + params.cubeSize) / params.cellSize);
    let cell_z = u32((pos.z + params.cubeSize) / params.cellSize);

    let cell_hash = (cell_z * params.gridHeight + cell_y) * params.gridWidth + cell_x; // номер ячейки в сетке в которую попадает боид

    let local_index = atomicAdd(&gridCounters[cell_hash], 1u);

    if (local_index < MAX_BOIDS_PER_CELL) {
        gridCells[cell_hash * MAX_BOIDS_PER_CELL + local_index] = boid_index;
    }
}


@compute @workgroup_size(64)
fn clear_grid(@builtin(global_invocation_id) global_id: vec3<u32>) {
    let cell_index = global_id.x;

    if (cell_index >= arrayLength(&gridCounters)) {
        return;
    }

    atomicStore(&gridCounters[cell_index], 0u);
}