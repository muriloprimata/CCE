import sys, os, time, numpy as np, chess, torch, torch.nn as nn
torch.set_num_threads(os.cpu_count())
H1, H2, T0, CAP = 256, 32, time.time(), 8000000
n = 0
for _ in open(sys.argv[1]):
    n += 1
    if n >= CAP: break
X = np.full((n, 32), 768, dtype=np.int16); Yl = np.zeros(n, dtype=np.float32)
for i, line in enumerate(open(sys.argv[1])):
    if i >= n: break
    fen, s = line.strip().rsplit(",", 1)
    a = [(p.piece_type - 1 + (0 if p.color else 6)) * 64 + sq for sq, p in chess.Board(fen).piece_map().items()]
    X[i, :len(a)] = a; Yl[i] = float(s)
X = torch.from_numpy(X); Y = torch.sigmoid(torch.from_numpy(Yl) / 400)
print("posicoes:", n, flush=True)
k = int(n * 0.98)
ft = nn.EmbeddingBag(769, H1, mode="sum", padding_idx=768)
nn.init.normal_(ft.weight, 0, 0.05); ft.weight.data[768] = 0
b1 = nn.Parameter(torch.zeros(H1))
l2, l3 = nn.Linear(H1, H2), nn.Linear(H2, 1)
def fwd(x):
    h = torch.clamp(ft(x.long()) + b1, 0, 1)
    h = torch.clamp(l2(h), 0, 1)
    return torch.sigmoid(l3(h).squeeze(1))
opt = torch.optim.Adam(list(ft.parameters()) + [b1] + list(l2.parameters()) + list(l3.parameters()), lr=1e-3)
sched = torch.optim.lr_scheduler.ExponentialLR(opt, 0.95)
for ep in range(60):
    perm = torch.randperm(k)
    for i in range(0, k, 4096):
        j = perm[i:i+4096]
        loss = nn.functional.mse_loss(fwd(X[j]), Y[j])
        opt.zero_grad(); loss.backward(); opt.step()
    sched.step()
    with torch.no_grad():
        t = nn.functional.mse_loss(fwd(X[k:]), Y[k:]).item()
    print(f"epoca {ep+1}: teste {t:.5f}", flush=True)
    if time.time() - T0 > 17000: break
with open(sys.argv[2], "wb") as f:
    for w in (ft.weight[:768], b1, l2.weight, l2.bias, l3.weight, l3.bias):
        f.write(w.detach().numpy().astype(np.float32).tobytes())
