final lookIdPattern = RegExp(r'^[a-z0-9]+(-[a-z0-9]+)*$');

bool isValidLookId(String id) =>
    id != 'builtin' && lookIdPattern.hasMatch(id);

bool isReservedLookId(String id) => id == 'builtin';
