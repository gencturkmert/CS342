import matplotlib.pyplot as plt

# Data
f_values = [4, 8, 16, 32, 64, 128]
clock = [822, 692, 374, 133, 100, 82]
eclock = [814, 676, 323, 163, 126, 82]
fifo = [819, 682, 365, 133, 100, 82]
lru = [813, 682, 347, 117, 82, 82]

# Plotting for all algorithms in one plot
plt.figure(figsize=(10, 6))

# Plot for Clock
plt.plot(f_values, clock, label='Clock')

# Plot for Enhanced Clock (eclock)
plt.plot(f_values, eclock, label='Enhanced Clock')

# Plot for FIFO
plt.plot(f_values, fifo, label='FIFO')

# Plot for LRU
plt.plot(f_values, lru, label='LRU')

# Adding labels and title
plt.xlabel('Frame Count')
plt.ylabel('Page Faults')
plt.title('Page Faults vs. Frame Count for Different Algorithms')
plt.legend()
plt.grid(True)

# Display the plot
plt.show()

# Separate plots for each algorithm
plt.figure(figsize=(10, 6))

# Plot for Clock
plt.plot(f_values, clock, label='Clock')
plt.xlabel('Frame Count')
plt.ylabel('Page Faults')
plt.title('Clock Algorithm')
plt.grid(True)
plt.show()

# Plot for Enhanced Clock (eclock)
plt.figure(figsize=(10, 6))
plt.plot(f_values, eclock, label='Enhanced Clock')
plt.xlabel('Frame Count')
plt.ylabel('Page Faults')
plt.title('Enhanced Clock Algorithm')
plt.grid(True)
plt.show()

# Plot for FIFO
plt.figure(figsize=(10, 6))
plt.plot(f_values, fifo, label='FIFO')
plt.xlabel('Frame Count')
plt.ylabel('Page Faults')
plt.title('FIFO Algorithm')
plt.grid(True)
plt.show()

# Plot for LRU
plt.figure(figsize=(10, 6))
plt.plot(f_values, lru, label='LRU')
plt.xlabel('Frame Count')
plt.ylabel('Page Faults')
plt.title('LRU Algorithm')
plt.grid(True)
plt.show()
