# Security Policy

## Reporting Security Issues

**Do not open a public GitHub issue for security vulnerabilities.**

Instead, please email security concerns to the project maintainers. For now, use GitHub's private vulnerability reporting if available in your region.

Include:
- Description of the vulnerability
- Steps to reproduce
- Potential impact
- Suggested fix (if any)

## Security Best Practices for Users

### 1. Enable Authentication

In your device settings, enable HTTP Basic Authentication:
- Prevents unauthorized access to `/update` and `/fs` endpoints
- Protects against remote firmware modification
- Required for production deployments

### 2. Use HTTPS in Production

If deploying publicly:
- Use reverse proxy (nginx, Caddy) with HTTPS
- Never expose HTTP directly to the internet
- Consider VPN for remote access

### 3. Strong WiFi Password

- Use WPA2/WPA3 encryption
- Strong password (20+ characters)
- Change from default

### 4. Network Isolation

- Place device on separate VLAN if possible
- Use firewall rules to limit access
- Only expose required ports (WebSocket on 80/443)

### 5. Regular Updates

- Keep firmware updated
- Check for security patches
- Test updates before deploying to production

## Security Features

### Current Implementation

**Authentication**:
- Optional HTTP Basic Auth for `/update`, `/fs`, `/recovery`
- Controlled by `settings.authMode` parameter
- Credentials stored in EEPROM

**WebSocket**:
- Binary protocol with struct validation
- No plaintext command transmission
- Ping/pong heartbeat for connection health

### Known Limitations

⚠️ **These are NOT security features**:
- WebSocket lacks message authentication
- No encryption by default (HTTP/WS not HTTPS/WSS)
- No rate limiting on requests
- No CSRF protection

⚠️ **Authentication is optional** - must be explicitly enabled

## Security Roadmap

### Recommended Improvements

1. **Message Authentication**
   - Add HMAC or signature to WebSocket messages
   - Verify message integrity and origin

2. **TLS/SSL Support**
   - Enable HTTPS/WSS for encrypted communication
   - Requires more flash storage (~500KB)

3. **Token-Based Auth**
   - Replace HTTP Basic Auth with JWT tokens
   - Implement token expiration
   - Support multiple user roles

4. **Rate Limiting**
   - Limit requests per IP/user
   - Prevent brute-force attacks
   - Implement exponential backoff

5. **Input Validation**
   - Strict bounds checking on all inputs
   - Sanitize file paths (prevent directory traversal)
   - Validate struct sizes

## Implementation Status

- ✅ HTTP Basic Auth (optional)
- ✅ Binary WebSocket protocol
- ⚠️ Buffer overflow protection (strncpy instead of strcpy)
- ❌ TLS/SSL support
- ❌ Message authentication/signing
- ❌ Rate limiting
- ⚠️ Path traversal prevention (partial)

## For Developers

### Secure Coding Guidelines

1. **Use strncpy instead of strcpy**
   ```cpp
   strncpy(dest, src, sizeof(dest) - 1);
   dest[sizeof(dest) - 1] = '\0';
   ```

2. **Validate all inputs**
   ```cpp
   if (size > MAX_SIZE || size < MIN_SIZE) return ERROR;
   ```

3. **Check WebSocket client status**
   ```cpp
   if (ws.hasClient(clientID)) {
       ws.binary(clientID, buffer, len);
   }
   ```

4. **Prevent directory traversal**
   ```cpp
   // Don't allow ../ in file paths
   if (strstr(path, "..")) return ERROR;
   ```

5. **Escape user input in HTML**
   - Frontend uses Vue which auto-escapes by default
   - Be explicit with `v-html` (audit carefully)

### Testing Security

Before submitting changes:

1. **Type safety**: `npm run typecheck`
2. **Code quality**: `npm run lint`
3. **Buffer bounds**: Manual code review
4. **Input validation**: Test with malformed data
5. **Device testing**: Test on actual hardware

## Compliance

This project aims to be:
- ✅ Free of buffer overflows (strncpy usage)
- ⚠️ Minimal CWE vulnerabilities
- ❌ OWASP Top 10 compliant (in progress)

## Contact

For security-related questions or responsible disclosure:
- Open a private GitHub security advisory if available
- Contact project maintainers directly

## Acknowledgments

Thank you to security researchers who responsibly disclose vulnerabilities. Your help makes Vuesp safer for everyone.
