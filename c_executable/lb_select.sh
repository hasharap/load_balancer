#!/bin/sh


# Create the load_balancer directory in /tmp
mkdir -p /tmp/load_balancer
#touch /tmp/load_balancer/load_balancer_state
# Define the file path
LB_FILE="/charger-flags/lb_type"

# Check if lb_type file exists
if [ ! -f "$LB_FILE" ]; then
  echo "Error: $LB_FILE file not found"
  exit 1
fi

# Read the value from lb_type file
lb_value=$(cat "$LB_FILE")

# Function to run and respawn the executable
run_and_respawn() {
  local executable=$1
  while true; do
    echo "Starting $executable..."
    $executable
    echo "$executable exited with status $?. Respawning in 1 second..."
    sleep 1
  done
}

# Check the value and execute the corresponding executable with respawn
if [ "$lb_value" = "1" ]; then
  chmod +x /chargerFiles/L2/load_balancing_type_1
  run_and_respawn "/chargerFiles/L2/load_balancing_type_1"
elif [ "$lb_value" = "2" ]; then
  chmod +x /chargerFiles/L2/load_balancing_type_2
  run_and_respawn "/chargerFiles/L2/load_balancing_type_2"
else
  echo "Load Balancing Function Desabled!..., got $lb_value"
  exit 1
fi
