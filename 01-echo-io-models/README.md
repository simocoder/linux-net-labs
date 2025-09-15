# 01 — Echo I/O Models (blocking → select → epoll)

**Day 1 scope:** blocking echo server (single client at a time), clean shutdown, quick manual test.

## Definition of Done (Day 1)
- Build with `make`
- Manual test with `nc` shows echo
- Graceful Ctrl-C shutdown
- Notes on limitations (single client)

## Build & Run
```bash
cd blocking
make
./run.sh 8080
# new shell:
nc 127.0.0.1 8080
hello
hello
