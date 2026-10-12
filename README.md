# The server application for [scheda-informationis](https://github.com/JRD-HTLWY/scheda-informationis)

All tickets are saved to <> in a json-array in the following format:

```json
{
  "id": {
    "title": "STRING",
    "status": "INT",
    "description": "STRING"
  }
}
```

Status is an enum and can have the following states:
- 0: unveryfied
- 1: verfied
- 2: in progress
- 3: resolved
- 4: dropped
