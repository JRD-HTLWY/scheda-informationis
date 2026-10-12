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
- 0: unverified
- 1: verified
- 2: in progress
- 3: resolved
- 4: dropped


Technologies used in this project:
- wxWidgets
- make
- JSON
- CMAKE
- jsoncons
- asio
- C++
- wxFormBuilder