import time
import sys
import signal
import psutil

# Flag to track if we should stop allocating
stop_allocating = False

def signal_handler(signum, frame):
    """Handle termination signals gracefully"""
    global stop_allocating
    print(f"\nReceived signal {signum}, stopping allocation...")
    stop_allocating = True

def allocate_memory():
    """Allocate memory in 100MB chunks until OOM"""
    global stop_allocating
    
    # Register signal handlers
    signal.signal(signal.SIGTERM, signal_handler)
    signal.signal(signal.SIGINT, signal_handler)
    
    chunks = []
    chunk_size = 100 * 1024 * 1024  # 100 MB in bytes
    chunk_number = 0
    
    print("Starting memory allocation...")
    print(f"Chunk size: 100 MB")
    
    # Get system memory info
    mem = psutil.virtual_memory()
    max_safe_mb = int(mem.available * 0.85 / (1024 * 1024))  # Use only 85% of available
    print(f"Available memory: {mem.available / (1024**3):.2f} GB")
    print(f"Will allocate up to ~{max_safe_mb} MB to avoid OOM killer\n")
    
    try:
        while not stop_allocating:
            # Check if we're approaching memory limits
            mem = psutil.virtual_memory()
            if mem.percent > 95 or chunk_number * 100 >= max_safe_mb:
                print(f"\nReached safe memory limit (RAM usage: {mem.percent}%)")
                print(f"Allocated {chunk_number} chunks = {chunk_number * 100} MB")
                stop_allocating = True
                break
            
            # Allocate 100MB chunk (fill with data to actually use memory)
            chunk = bytearray(chunk_size)
            # Write to the memory to ensure it's actually allocated
            for i in range(0, chunk_size, 4096):
                chunk[i] = 1
            
            chunks.append(chunk)
            chunk_number += 1
            total_mb = chunk_number * 100
            
            mem = psutil.virtual_memory()
            print(f"Allocated chunk #{chunk_number} | Total: {total_mb} MB | RAM: {mem.percent:.1f}%")
            
    except MemoryError:
        print(f"\n!!! Out of Memory !!!")
        print(f"Successfully allocated {chunk_number} chunks = {chunk_number * 100} MB")
    
    # Hold the memory and print status every second
    print(f"\nNow holding memory and sleeping...")
    try:
        while True:
            mem = psutil.virtual_memory()
            print(f"Holding {chunk_number * 100} MB | Chunks: {len(chunks)} | RAM: {mem.percent:.1f}%")
            time.sleep(1)
    except KeyboardInterrupt:
        print(f"\nInterrupted! Allocated {chunk_number * 100} MB before exit")
        sys.exit(0)

if __name__ == "__main__":
    allocate_memory()