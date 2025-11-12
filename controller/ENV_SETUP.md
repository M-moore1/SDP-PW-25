# Environment Setup

## Creating the .env File

The `.env` file is gitignored for security reasons. You need to create it manually.

### Quick Setup

Run one of these commands in the `controller` directory:

**Option 1: Using the install script**
```bash
./INSTALL.sh
```

**Option 2: Manual creation**
```bash
echo "VITE_WS_URL=ws://localhost:8080" > .env
```

**Option 3: Copy from example**
```bash
cp .env.example .env
```

### .env File Contents

```env
# WebSocket URL for the controller
# Change this to match your WebSocket server address
VITE_WS_URL=ws://localhost:8080
```

### Different Environments

#### Local Development (default)
```env
VITE_WS_URL=ws://localhost:8080
```

#### Production Server
```env
VITE_WS_URL=wss://your-production-server.com/ws
```

#### Custom Port
```env
VITE_WS_URL=ws://localhost:3001
```

#### Secure WebSocket (wss://)
```env
VITE_WS_URL=wss://secure-server.com:443/ws
```

### Important Notes

1. **File is required**: The app will use `ws://localhost:8080` as fallback if `.env` is missing
2. **Restart required**: After changing `.env`, restart the dev server (`npm run dev`)
3. **Not committed**: This file is in `.gitignore` and won't be committed to git
4. **Build time**: Environment variables are embedded at build time, not runtime

### Verifying Configuration

After creating `.env`, you can verify it's working by:

1. Starting the app: `npm run dev`
2. Opening browser console (F12)
3. The connection status should show the URL you configured
4. Check the Status panel in the UI for the WebSocket URL

### Troubleshooting

**Problem**: Changes to .env not taking effect
- **Solution**: Stop the dev server (Ctrl+C) and restart with `npm run dev`

**Problem**: WebSocket won't connect
- **Solution**: Verify the URL in `.env` matches your server address
- Check that your WebSocket server is running
- Check browser console for error messages

**Problem**: .env file not found errors
- **Solution**: Make sure you're in the `controller` directory when creating `.env`
- Check that the filename is exactly `.env` (with the dot, no extension)

### Security Best Practices

1. **Never commit** `.env` to version control
2. **Use environment-specific** files for different deployments
3. **Document** required variables in `.env.example`
4. **For production**, use your hosting platform's environment variable system

