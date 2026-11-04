import matplotlib.pyplot as plt
import numpy as np
import random
from collections import deque
import time

# --- Maze Constants ---
NUM_RINGS = 6
INNER_RADIUS = 1.0
RING_STEP = 1.0

class MazeGame:
    def __init__(self, num_rings=8):
        self.num_rings = num_rings
        self.sectors_per_ring = []
        self.walls = {}
        self.maze_adj = {}
        self.exit_cell = (0, 0)
        self.player_r, self.player_s = 0, 0
        self.prev_player_r, self.prev_player_s = 0, 0
        self.visual_radius = INNER_RADIUS + RING_STEP / 2.0
        self.visual_angle = 0.0
        self.available_options = []
        self.selected_idx = 0
        self.is_moving = False
        self.is_won = False
        self.generate()
        self.update_options()
        self.visual_angle = (self.player_s + 0.5) * (2 * np.pi / self.sectors_per_ring[0])

    def sector_mod(self, s, n): return (s % n + n) % n

    def generate(self):
        current_s = 12
        for r in range(self.num_rings):
            self.sectors_per_ring.append(current_s)
            if (r + 1) % 3 == 0 and r < self.num_rings - 1: current_s *= 2
        for r in range(self.num_rings):
            n_s = self.sectors_per_ring[r]
            for s in range(n_s):
                self.walls[tuple(sorted(((r, s), (r, self.sector_mod(s + 1, n_s)))))] = True
                if r < self.num_rings - 1:
                    ratio = self.sectors_per_ring[r+1] // n_s
                    for i in range(ratio): self.walls[tuple(sorted(((r, s), (r + 1, s * ratio + i))))] = True
        visited = set()
        stack = [((0, 0), 'start')]
        visited.add((0, 0))
        self.maze_adj = { (r, s): [] for r in range(self.num_rings) for s in range(self.sectors_per_ring[r]) }
        while stack:
            curr, prev_type = stack[-1]
            r, s = curr
            n_s = self.sectors_per_ring[r]
            neighbors = []
            for ns in [self.sector_mod(s-1, n_s), self.sector_mod(s+1, n_s)]:
                if (r, ns) not in visited: neighbors.append(((r, ns), 'sideways'))
            if prev_type != 'radial':
                if r > 0:
                    in_c = (r-1, s // (n_s // self.sectors_per_ring[r-1]))
                    if in_c not in visited: neighbors.append((in_c, 'radial'))
                if r < self.num_rings - 1:
                    ratio = self.sectors_per_ring[r+1] // n_s
                    for i in range(ratio):
                        out_c = (r+1, s*ratio + i)
                        if out_c not in visited: neighbors.append((out_c, 'radial'))
            if neighbors:
                next_cell, n_type = random.choice(neighbors)
                wall_key = tuple(sorted((curr, next_cell)))
                if wall_key in self.walls: del self.walls[wall_key]
                self.maze_adj[curr].append(next_cell); self.maze_adj[next_cell].append(curr)
                visited.add(next_cell); stack.append((next_cell, n_type))
            else: stack.pop()
        distances = {(0, 0): 0}
        queue = deque([(0, 0)])
        while queue:
            c = queue.popleft()
            for nb in self.maze_adj[c]:
                if nb not in distances: distances[nb] = distances[c] + 1; queue.append(nb)
        outer = [(self.num_rings - 1, s) for s in range(self.sectors_per_ring[-1])]
        self.exit_cell = max(outer, key=lambda c: distances.get(c, -1))

    def update_options(self):
        r, s = self.player_r, self.player_s
        n_s = self.sectors_per_ring[r]
        options = []
        if tuple(sorted(((r, s), (r, self.sector_mod(s + 1, n_s))))) not in self.walls:
            options.append({'dir': 'CW', 'cell': (r, self.sector_mod(s + 1, n_s)), 'offset': 0.0})
        if tuple(sorted(((r, s), (r, self.sector_mod(s - 1, n_s))))) not in self.walls:
            options.append({'dir': 'CCW', 'cell': (r, self.sector_mod(s - 1, n_s)), 'offset': 0.0})
        if r > 0:
            in_c = (r-1, s // (n_s // self.sectors_per_ring[r-1]))
            if tuple(sorted(((r, s), in_c))) not in self.walls: options.append({'dir': 'IN', 'cell': in_c, 'offset': 0.0})
        if r < self.num_rings - 1:
            ratio = self.sectors_per_ring[r+1] // n_s
            for i in range(ratio):
                out_c = (r+1, s*ratio + i)
                if tuple(sorted(((r, s), out_c))) not in self.walls:
                    options.append({'dir': 'OUT', 'cell': out_c, 'offset': (i + 0.5) / ratio - 0.5})
        elif (r, s) == self.exit_cell:
            options.append({'dir': 'OUT', 'cell': 'win', 'offset': 0.0})
        self.available_options = options

    def step(self):
        if not self.is_moving or not self.available_options: return
        
        opt = self.available_options[self.selected_idx % len(self.available_options)]
        next_cell = opt['cell']
        
        if next_cell == 'win':
            self.is_moving = False; self.is_won = True; return

        # Collision check
        if tuple(sorted(((self.player_r, self.player_s), next_cell))) in self.walls:
            self.is_moving = False; self.update_options(); return
            
        self.prev_player_r, self.prev_player_s = self.player_r, self.player_s
        self.player_r, self.player_s = next_cell
        self.update_options()
        total = len(self.available_options)
        
        # Stop at Jumps, Junctions, or Dead Ends
        if self.player_r != self.prev_player_r or total != 2:
            self.is_moving = False
            if total > 1:
                back = (self.prev_player_r, self.prev_player_s)
                for i, o in enumerate(self.available_options):
                    if o['cell'] == back:
                        self.selected_idx = (i + 1) % total; break
            else: self.selected_idx = 0
        else: # Corridor: follow forward
            back = (self.prev_player_r, self.prev_player_s)
            for i, o in enumerate(self.available_options):
                if o['cell'] != back:
                    self.selected_idx = i; break

# --- Visualization ---
game = MazeGame(NUM_RINGS)
plt.ion(); fig, ax = plt.subplots(subplot_kw={'projection': 'polar'}, figsize=(8, 8))

def draw_static_maze():
    ax.clear(); ax.set_axis_off()
    for (c1, c2) in game.walls.keys():
        r1, s1 = c1; r2, s2 = c2
        if r1 == r2:
            n_s = game.sectors_per_ring[r1]
            theta = 2 * np.pi * max(s1, s2) / n_s
            if s1 == 0 and s2 == n_s - 1: theta = 0
            ax.plot([theta, theta], [INNER_RADIUS + r1*RING_STEP, INNER_RADIUS + (r1+1)*RING_STEP], color='black', lw=2.5)
        else:
            r_v = INNER_RADIUS + r2 * RING_STEP; n_o = game.sectors_per_ring[r2]
            t_s, t_e = 2 * np.pi * s2 / n_o, 2 * np.pi * (s2 + 1) / n_o
            ax.plot(np.linspace(t_s, t_e, 30), [r_v]*30, color='black', lw=2.5)
    ax.plot(np.linspace(0, 2*np.pi, 100), [INNER_RADIUS]*100, color='black', lw=2.5)
    r_l, n_l = INNER_RADIUS + NUM_RINGS * RING_STEP, game.sectors_per_ring[-1]
    for s in range(n_l):
        if (NUM_RINGS-1, s) == game.exit_cell: continue
        t_s, t_e = 2 * np.pi * s / n_l, 2 * np.pi * (s + 1) / n_l
        ax.plot(np.linspace(t_s, t_e, 30), [r_l]*30, color='black', lw=2.5)
    ax.text(2 * np.pi * (game.exit_cell[1] + 0.5) / n_l, r_l + 0.4, 'EXIT', color='green', fontweight='bold', ha='center', va='center')
    ax.set_ylim(0, r_l + 1.5)

draw_static_maze()
player_dot, = ax.plot([], [], 'yo', ms=8, zorder=10, markeredgecolor='orange')
selector_dot, = ax.plot([], [], 'ro', ms=4, zorder=11)

def on_key(event):
    if event.key == 'right':
        if not game.is_moving and game.available_options and not game.is_won:
            game.selected_idx = (game.selected_idx + 1) % len(game.available_options)
    elif event.key == 'left':
        if not game.is_moving and game.available_options and not game.is_won:
            game.selected_idx = (game.selected_idx - 1) % len(game.available_options)
    elif event.key == ' ':
        if not game.is_moving and game.available_options and not game.is_won:
            game.is_moving = True

fig.canvas.mpl_connect('key_press_event', on_key)
print("\n--- CIRCULAR MAZE SIMULATOR ---\nARROWS to select, SPACE to move.")

try:
    while plt.fignum_exists(fig.number):
        t_r = INNER_RADIUS + game.player_r * RING_STEP + RING_STEP/2.0
        if game.is_won: t_r = INNER_RADIUS + NUM_RINGS * RING_STEP + 0.8
        t_a = 2 * np.pi * (game.player_s + 0.5) / game.sectors_per_ring[game.player_r]
        if game.visual_angle - t_a > np.pi: game.visual_angle -= 2*np.pi
        elif t_a - game.visual_angle > np.pi: game.visual_angle += 2*np.pi
        
        # Arrival check
        dist_sq = (game.visual_radius - t_r)**2 + (game.visual_angle - t_a)**2
        arrived = dist_sq < 0.002
        
        if arrived and game.is_moving:
            game.step() # Advanced logic only after visual dot arrives
        
        # Slower smoothing to keep dot on path
        game.visual_radius += (t_r - game.visual_radius) * 0.2
        game.visual_angle += (t_a - game.visual_angle) * 0.2
        player_dot.set_data([game.visual_angle], [game.visual_radius])
        
        if not game.is_moving and arrived and game.available_options and not game.is_won:
            opt = game.available_options[game.selected_idx % len(game.available_options)]
            sa, sr = game.visual_angle, game.visual_radius
            n_curr = game.sectors_per_ring[game.player_r]
            angle_off = (2 * np.pi / n_curr) * 0.3
            if opt['dir'] == 'CW': sa += angle_off
            elif opt['dir'] == 'CCW': sa -= angle_off
            elif opt['dir'] == 'OUT': sr += 0.6; sa += opt.get('offset', 0.0) * (2 * np.pi / n_curr) * 0.4
            elif opt['dir'] == 'IN': sr -= 0.6
            selector_dot.set_data([sa], [sr])
        else: selector_dot.set_data([], [])
        
        fig.canvas.draw_idle(); fig.canvas.start_event_loop(0.01)
        if game.is_won and arrived:
            print("\n!!! CONGRATULATIONS !!!\nYou escaped the maze."); time.sleep(2); break
except KeyboardInterrupt: pass
