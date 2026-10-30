import sys, time, random, chess, chess.engine
secs = int(sys.argv[1]); out = sys.argv[2]; D = int(sys.argv[3])
t0 = time.time()
sf = chess.engine.SimpleEngine.popen_uci("stockfish")
sf.configure({"Threads": 1, "Hash": 16})
f = open(out, "a"); got = 0
while time.time() - t0 < secs:
    b = chess.Board()
    for ply in range(100):
        if b.is_game_over() or time.time() - t0 >= secs: break
        if random.random() < 0.25:
            m = random.choice(list(b.legal_moves))
        else:
            m = sf.play(b, chess.engine.Limit(nodes=300)).move
        b.push(m)
        if ply >= 8 and ply % 3 == 0 and not b.is_check():
            s = sf.analyse(b, chess.engine.Limit(depth=D))["score"].white().score(mate_score=1000)
            s = max(-1000, min(1000, s))
            f.write(f"{b.fen()},{s}\n"); got += 1
    f.flush()
    print(got, flush=True)
sf.quit()
