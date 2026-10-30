import sys, chess, chess.engine

cart_path = sys.argv[1]            # caminho da engine (ex: ./cartman)
elo   = int(sys.argv[2])           # UCI_Elo do Stockfish (>= 1320)
games = int(sys.argv[3])           # numero de partidas
mt    = float(sys.argv[4])         # segundos por lance

sf = chess.engine.SimpleEngine.popen_uci("stockfish")
sf.configure({"Threads": 1, "Hash": 16,
              "UCI_LimitStrength": True, "UCI_Elo": elo})
ct = chess.engine.SimpleEngine.popen_uci(cart_path)

w = d = l = 0
try:
    for g in range(games):
        board = chess.Board()
        cart_white = (g % 2 == 0)
        while not board.is_game_over(claim_draw=True) and board.fullmove_number < 200:
            cart_turn = (board.turn == chess.WHITE) == cart_white
            eng = ct if cart_turn else sf
            r = eng.play(board, chess.engine.Limit(time=mt))
            board.push(r.move)
        res = board.result(claim_draw=True)
        if res == "1/2-1/2" or res == "*":
            d += 1
        elif (res == "1-0") == cart_white:
            w += 1
        else:
            l += 1
        print(f"jogo {g+1}: {res}  | V{w} E{d} D{l}", flush=True)
finally:
    sf.quit()
    ct.quit()

pts = (w + d / 2) / max(1, games)
print(f"Cartman: {w}V {d}E {l}D  ({pts*100:.0f}% dos pontos) vs SF {elo}")
