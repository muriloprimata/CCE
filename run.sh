#!/data/data/com.termux/files/usr/bin/bash
cd ~/cartman
id() { gh run list --workflow=treino.yml --limit 1 --json databaseId -q '.[0].databaseId'; }
case "$1" in
  start)
    git add -A; git commit -qm "treino"
    git push || exit 1
    sleep 10
    gh workflow run treino.yml && sleep 8 && gh run list --workflow=treino.yml --limit 1
    ;;
  status) gh run view "$(id)" ;;
  baixar)
    rm -rf resultados
    gh run download "$(id)" -D resultados -p 'res-*' -p 'match-*' -p 'nnue'
    echo "== depths (menor mse = melhor) =="
    cat resultados/res-d*/res-*.txt 2>/dev/null | sort -t= -k4 -g
    echo "== Cartman (avaliacao simples) vs Stockfish =="
    grep -h '^Cartman:' resultados/match-*/out.txt 2>/dev/null | sort -t' ' -k8 -n
    if [ -f resultados/nnue/nnue.bin ]; then cp resultados/nnue/nnue.bin net/nnue.bin; echo "nnue.bin copiado pra net/"; fi
    ;;
  *) echo "uso: ./run.sh start | status | baixar" ;;
esac
