# Contributing to Vuesp

Thank you for your interest in contributing! This document provides guidelines for contributing to the Vuesp project.

## Code of Conduct

- Be respectful and inclusive
- Focus on the issue, not the person
- Ask questions if something is unclear
- Help others when possible

## Getting Started

1. **Fork the repository** on GitHub
2. **Clone your fork**:
   ```bash
   git clone https://github.com/YOUR_USERNAME/vuesp.git
   cd vuesp
   ```
3. **Create a feature branch**:
   ```bash
   git checkout -b feat/my-feature
   ```

## Development Workflow

### Before You Start

1. Read [DEVELOPMENT.md](./DEVELOPMENT.md)
2. Set up environment: `npm install` in `vue/` directory
3. Set `VITE_IP` in `.env` to your device IP

### Making Changes

1. **Make small, focused commits**
   - One logical change per commit
   - Write clear commit messages (see below)

2. **Keep code consistent**
   - Frontend: Run `npm run format` and `npm run lint`
   - Backend: Follow existing code style
   - Use TypeScript for type safety

3. **Test your changes**
   - Build frontend: `npm run build-web`
   - Type check: `npm run typecheck`
   - Test on actual device before submitting

### Commit Messages

Format:
```
type(scope): subject

Body (optional, explain why not what)

Footer (optional, reference issues)
```

Types:
- `feat` - New feature
- `fix` - Bug fix
- `docs` - Documentation
- `style` - Formatting, missing semicolons, etc
- `refactor` - Code change without features/fixes
- `perf` - Performance improvement
- `test` - Adding tests
- `chore` - Build, deps, tooling

Examples:
```
feat(ui): add dark mode toggle

Implement theme switching in App.vue and persist to localStorage.
Fixes #42

---

fix(webserver): prevent buffer overflow in file upload

Use strncpy instead of strcpy in onReqUpload handler.

---

docs: update setup instructions

Add troubleshooting section and clarify VITE_IP configuration.
```

## Submitting Changes

### Before Submitting PR

1. **Run checks**:
   ```bash
   cd vue
   npm run check
   npm run typecheck
   npm run build-web
   ```

2. **Update documentation** if needed:
   - README.md for user-facing changes
   - DEVELOPMENT.md for development changes
   - Comments for complex logic

3. **Test thoroughly**:
   - Test on target device
   - Try different platforms (esp32, esp8266, pico32)
   - Test both with and without authentication

### Pull Request Template

```markdown
## Description
Brief description of changes

## Motivation & Context
Why is this change needed? What problem does it solve?

## Types of changes
- [ ] Bug fix (non-breaking change)
- [ ] New feature (non-breaking change)
- [ ] Breaking change
- [ ] Documentation update

## Testing
How was this tested?
- [ ] Manual testing on device
- [ ] TypeScript checks pass
- [ ] ESLint checks pass
- [ ] Build succeeds

## Checklist
- [ ] Code follows project style
- [ ] Comments added for complex logic
- [ ] Documentation updated
- [ ] No new compiler warnings
```

## Code Review

### What Gets Reviewed

- **Security**: No vulnerabilities or unsafe patterns
- **Performance**: No unnecessary work or blocking operations
- **Compatibility**: Works on supported platforms
- **Style**: Matches project conventions
- **Testing**: Properly tested

### Making Requested Changes

1. Make changes on your branch
2. Push updates: `git push origin feat/my-feature`
3. PR will auto-update

## Common Issues

### Build Fails in CI

- Clear cache: `rm -rf build node_modules`
- Rebuild: `npm install && npm run build-web`
- Check error messages for specifics

### TypeScript Errors

Run locally:
```bash
npm run typecheck
```

Fix issues in code, not in config files.

### ESLint Violations

Auto-fix:
```bash
npm run lint
```

Review changes to ensure they're correct.

## Special Types of Contributions

### Bug Reports

1. Check if already reported
2. Include:
   - Exact error message
   - Steps to reproduce
   - Device info (ESP32, ESP8266, etc)
   - Firmware version
   - Browser console errors

### Feature Requests

1. Check if already requested
2. Describe:
   - What feature you want
   - Why it's useful
   - How it should work
   - Any alternatives you considered

### Documentation

- Fixes typos and clarifications are always welcome
- Add examples for complex features
- Ensure code examples are correct and up-to-date

## Questions?

- Open a Discussion on GitHub
- Check existing issues and PRs
- Read [DEVELOPMENT.md](./DEVELOPMENT.md) first

## Recognition

Contributors will be recognized in:
- CHANGELOG.md for significant changes
- GitHub "Contributors" list

Thank you for making Vuesp better! 🎉
