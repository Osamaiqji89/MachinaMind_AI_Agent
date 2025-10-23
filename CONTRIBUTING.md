# Contributing to MachinaMindAIAgent

Vielen Dank für Ihr Interesse an MachinaMindAIAgent! 

## Entwicklungs-Workflow

1. **Fork** das Repository
2. **Branch** erstellen: `git checkout -b feature/amazing-feature`
3. **Commit** mit klarer Message: `git commit -m 'feat: Add amazing feature'`
4. **Push**: `git push origin feature/amazing-feature`
5. **Pull Request** öffnen

## Code-Style

### Python
- Formatter: `black` (line-length=100)
- Linter: `ruff`
- Type Hints: mandatory
- Tests: `pytest` (min. 80% coverage)

### C++
- Formatter: `clang-format` (Google Style)
- Naming: UpperCamelCase (Klassen), lowerCamelCase (Methoden), m_ prefix (Member)
- Tests: GoogleTest / Qt Test

## Commit Messages

Nutzen Sie [Conventional Commits](https://www.conventionalcommits.org/):

- `feat:` Neues Feature
- `fix:` Bugfix
- `docs:` Dokumentation
- `test:` Tests
- `refactor:` Code-Refactoring
- `chore:` Build/Tooling

**Beispiele:**
```
feat: Add anomaly detection with IsolationForest
fix: Resolve memory leak in ApiClient
docs: Update architecture diagram
```

## Testing

Vor jedem PR:

```powershell
# Python Tests
cd backend
pytest tests/ -v --cov=backend

# C++ Tests
cd cpp_gui/build
ctest --output-on-failure
```

## Pull Request Checklist

- [ ] Code folgt Style Guide
- [ ] Tests hinzugefügt/aktualisiert
- [ ] Dokumentation aktualisiert
- [ ] CI Pipeline läuft grün
- [ ] Keine Merge-Konflikte

## Fragen?

Öffnen Sie ein Issue oder kontaktieren Sie die Maintainer.
