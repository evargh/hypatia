import numpy as np
import matplotlib.pyplot as plt

# Parameters (example values for Earth's orbit)
a = 1.0  # semi-major axis in AU (astronomical units)
e = 0.0167  # eccentricity of Earth's orbit
i = np.radians(23.5)  # inclination in radians
omega = np.radians(102.9)  # argument of periapsis in radians
Omega = np.radians(0.0)  # longitude of ascending node in radians

# Generate theta (true anomaly) values
theta = np.linspace(0, 2 * np.pi, 1000)

# Compute radial distance r using the Kepler equation
r = a * (1 - e**2) / (1 + e * np.cos(theta))

# Compute Cartesian coordinates (x, y, z)
x = r * (
    np.cos(Omega) * np.cos(theta + omega)
    - np.sin(Omega) * np.sin(theta + omega) * np.cos(i)
)
y = r * (
    np.sin(Omega) * np.cos(theta + omega)
    + np.cos(Omega) * np.sin(theta + omega) * np.cos(i)
)
z = r * (np.sin(theta + omega) * np.sin(i))

# Plotting in 3D
fig = plt.figure(figsize=(10, 8))
ax = fig.add_subplot(111, projection="3d")

ax.plot(x, y, z, label="Orbit", color="b")
ax.set_xlabel("X")
ax.set_ylabel("Y")
ax.set_zlabel("Z")
ax.set_title("3D Keplerian Orbit")
ax.legend()

plt.show()
