class AsMixin:
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.short_name = None

    def as_(self, name):
        self.short_name = name
        return self
