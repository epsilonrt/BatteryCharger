# Contributing to BatteryCharger

Thank you for your interest in contributing to the BatteryCharger library!

## Development Workflow

### Branches

- **`main`**: Production release branch. Protected branch, only accepts PRs from `dev`.
- **`dev`**: Development branch. All feature PRs should target this branch first.

### Pull Request Process

1. **Create a feature branch** from `dev`:
   ```bash
   git checkout dev
   git pull origin dev
   git checkout -b feature/your-feature-name
   ```

2. **Make your changes** and commit with clear messages:
   ```bash
   git add .
   git commit -m "feat: add description of your changes"
   ```

3. **Push to your fork** and create a PR targeting `dev`:
   ```bash
   git push origin feature/your-feature-name
   ```

4. **CI will automatically run**:
   - Builds the example on all three supported boards (ESP32-C6, ESP32-S3, ESP32 classic)
   - Checks for compilation errors
   - The PR must pass all checks before review

5. **After approval**, your PR will be merged into `dev`

6. **Release to `main`**: When ready for a new release, a maintainer will create a PR from `dev` → `main` and tag a release

### Building Locally

Before submitting a PR, ensure builds pass on all environments:

```bash
cd examples/BatteryChargerSimpleTest
pio run -e makergo_c6_supermini
pio run -e waveshare_esp32_s3_zero
pio run -e adafruit_feather_esp32_v2
```

### Code Style

- Follow Arduino/C++ conventions
- Use meaningful variable names
- Add comments for complex logic
- Ensure your code compiles without warnings

### Reporting Issues

Please use GitHub Issues for bug reports. Include:
- Description of the issue
- Steps to reproduce
- Expected vs actual behavior
- Hardware/board information
