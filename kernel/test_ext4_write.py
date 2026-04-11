
img = "ext4fs/ext4fs.img" 
with open(img, 'rb') as f:
        # Seek to the start of sector 100 (512 bytes per sector)
    f.seek(1000 * 512)
    sector_data = f.read(512 * 2)
    print(f"data: {sector_data}")
