import sys, os, numpy as np, chess, torch, torch.nn as nn
torch.set_num_threads(os.cpu_count())
H1, H2 = 256, 32
def load(path, maxn=3000000):
    idx = []; Y = []
    for n, line in enumerate(open(path)):
        if n >= maxn: break
        fen, s = line.strip().rsplit(",", 1)
        b = chess.Board(fen)
        a = [(p.piece_type - 1 + (0 if p.color else 6)) * 64 + sq for sq, p in b.piece_map().items()]
        idx.append(a + [768] * (32 - len(a))); Y.append(float(s))
    return torch.from_numpy(np.array(idx, dtype=np.int16)), torch.sigmoid(torch.tensor(Y, dtype=torch.float32) / 400)
X, Y = load(sys.argv[1]); XT, YT = load(sys.argv[2]); depth = sys.argv[3]
print("treino:", len(X), "teste:", len(XT), flush=True)
ft = nn.EmbeddingBag(769, H1, mode="sum", padding_idx=768)
nn.init.normal_(ft.weight, 0, 0.05); ft.weight.data[768] = 0
b1 = nn.Parameter(torch.zeros(H1))
l2, l3 = nn.Linear(H1, H2), nn.Linear(H2, 1)
def fwd(x):
    h = torch.clamp(ft(x.long()) + b1, 0, 1)
    h = torch.clamp(l2(h), 0, 1)
    return torch.sigmoid(l3(h).squeeze(1))
opt = torch.optim.Adam(list(ft.parameters()) + [b1] + list(l2.parameters()) + list(l3.parameters()), lr=1e-3)
sched = torch.optim.lr_scheduler.ExponentialLR(opt, 0.9)
for ep in range(15):
    perm = torch.randperm(len(X))
    for i in range(0, len(X), 4096):
        j = perm[i:i+4096]
        loss = nn.functional.mse_loss(fwd(X[j]), Y[j])
        opt.zero_grad(); loss.backward(); opt.step()
    sched.step()
    with torch.no_grad():
        t = nn.functional.mse_loss(fwd(XT), YT).item()
    print(f"epoca {ep+1}: teste {t:.5f}", flush=True)
open(sys.argv[4], "w").write(f"depth={depth} pos={len(X)} mse={t:.5f}\n")
with open(sys.argv[5], "wb") as f:
    for w in (ft.weight[:768], b1, l2.weight, l2.bias, l3.weight, l3.bias):
        f.write(w.detach().numpy().astype(np.float32).tobytes())
