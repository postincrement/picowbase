#!/bin/bash
set -e

# Get the absolute path of the project directory
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CONTAINER_NAME="pico-w-builder"

# Function to check if container exists
container_exists() {
    podman ps -a --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"
}

# Function to check if container is running
container_running() {
    podman ps --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"
}

# Bootstrap command
bootstrap() {
    echo "Building container image..."
    podman build -t pico-w-builder .

    if container_exists; then
        echo "Removing existing container..."
        podman rm -f ${CONTAINER_NAME}
    fi

    echo "Creating new container..."
    podman create --name ${CONTAINER_NAME} -v "${PROJECT_DIR}:/build" localhost/pico-w-builder
    podman start ${CONTAINER_NAME}
    echo "Container created and started successfully!"
}

# Shell command
shell() {
    if ! container_exists; then
        echo "Container does not exist. Please run 'bootstrap' first."
        exit 1
    fi

    if ! container_running; then
        echo "Starting container..."
        podman start ${CONTAINER_NAME}
    fi

    echo "Entering container shell..."
    podman exec -it ${CONTAINER_NAME} /bin/bash
}

# Build command
build() {
    if ! container_exists; then
        echo "Container does not exist. Please run 'bootstrap' first."
        exit 1
    fi

    if ! container_running; then
        echo "Starting container..."
        podman start ${CONTAINER_NAME}
    fi

    echo "Running build script in container..."
    podman exec -it ${CONTAINER_NAME} /bin/bash -c "cd /build && ./build.sh"
}

# Program command
program() {
    if [ ! -f "build/pico_w_blink.elf" ]; then
        echo "ELF file not found. Please run 'build' first."
        exit 1
    fi

    echo "Programming Pico W..."
    picotool load -v build/pico_w_blink.elf

    echo "Rebooting Pico W into bootloader mode..."
    picotool reboot 
}

# Main command handling
case "$1" in
    "bootstrap")
        bootstrap
        ;;
    "shell")
        shell
        ;;
    "build")
        build
        ;;
    "program")
        program
        ;;
    *)
        echo "Usage: $0 {bootstrap|shell|build|program}"
        echo "  bootstrap : Build and start the container"
        echo "  shell     : Open a shell in the container"
        echo "  build     : Run the build script in the container"
        echo "  program   : Program the Pico W using picotool"
        exit 1
        ;;
esac 
