ECE Team

Tools & Environment
	Xilinx Vivado 2025.1: Hardware design and bitstream generation.
	Xilinx Vitis 2025.1: Software development and XSA processing.
	PetaLinux 2025.1: Custom embedded Linux build for ZCU102.
	PlatformIO (ESP-IDF): Firmware development for the ESP32.

Ground Station (GS)
The Ground Station serves as the bridge between the server and the robot, handling command parsing and secure communication.

	GS Build System
	Located in the /GS directory. It uses a centralized Makefile to manage the gs_bridge application.
		Commands: Use make rebuild clean to refresh the build.

	Components:
		BLE: Interface for robot communication.
		CMD Parsing: Logic for interpreting incoming instructions.
		JSON & UDS Setup: Data structuring and Unix Domain Socket configuration.
		GS Cryptography: Implementation of secure protocols.

Hardware (GS_Vivado)
Contains the Vivado project, Block Diagram (BD) images, and the exported .XSA file used as the hardware handoff for the PetaLinux build.

PetaLinux Build & OS Setup (Petalinux_Builds)
	1. SD Card Partitioning
		The ZCU102 requires a specific two-partition layout:
			Partition 1 (FAT32, ≥1 GB): Copy BOOT.BIN, image.ub, and boot.scr from SD_Card/Partion 1
			Partition 2 (ext4, ~15 GB): Flash the root filesystem extract rootfs.ext4 from SD_Card/Partition 2

	2. Post-Install: Node.js
		Due to RAM constraints during the VM build process, Node.js must be installed manually on the target hardware:	
			sudo wget https://nodejs.org/dist/v20.8.1/node-v20.8.1-linux-arm64.tar.xz
			sudo mkdir -p /usr/local/node
			sudo tar -xvf node-v20.8.1-linux-arm64.tar.xz -C /usr/local/node --strip-components=1
			echo 'export PATH=/usr/local/node/bin:$PATH' >> ~/.bashrc
			source ~/.bashrc

	3. Ethernet Troubleshooting
		If DHCP or connectivity fails on the ZCU102, run the following to reset the network stack:
			cat > /usr/lib/systemd/network/80-wired.network << EOF
			[Match]
			Type=ether
			Name=!veth*
			KernelCommandLine=!nfsroot
			KernelCommandLine=!ip

			[Network]
			DHCP=yes

			[DHCP]
			UseMTU=yes
			RouteMetric=10
			ClientIdentifier=mac
			EOF

			systemctl restart systemd-networkd

	4. Included OS Features
		The custom image comes pre-configured with:
			Nginx: Web server.
			Python3-pip: Package manager.
			TMUX: Terminal multiplexer (w/ glibc & locale).
			GCC: On-board C/C++ compiler.
			CSU Access: Secure register access via PMUFW.
			Weston/DP: Wayland compositor and DisplayPort output.
			UART1: J55 PMOD serial support (/dev/ttyPS2).
			Node.js: JavaScript runtime (v20.8.1).
			AES-GCM AXI IP: Hardware-accelerated 128/192/256-bit encryption.

Robot Firmware (robot)
	Developed for the ESP32 using the PlatformIO environment with the ESP-IDF framework.

	Directory Structure
		/components: Centralized library containing all custom drivers and logic modules.
		/test: Initial ESP-IDF project used for validation and unit testing.
		/Robot_Final: The production-ready design that integrates all components into the final firmware.

	Core Components (/components)
		ARM Code: Control logic for the robotic arm attachment.
		BLE Server: Wireless interface for Ground Station communication.
		Encryption/Decryption: CSE crypto library for secure data handling.
		I2C IMU: Telemetry and movement feedback via the IMU system.
		Pinout: Centralized macro header for rapid GPIO reconfiguration.
		Robot_Command: Logic engine for processing and executing incoming commands.
		Stepper_Motor: Struct-based controller for high-precision motor movement.

Documentation (icd document)
	Refer to the ICD (Interface Control Document) for the specific command structures and communication protocols used across the entire system.



