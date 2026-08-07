project = "libapfs"
author = "Avighna Chhatrapati and Oviyan Gandhi"
copyright = "2026, Avighna Chhatrapati and Oviyan Gandhi"
release = "1.0"

extensions = [
    "myst_parser",
    "breathe",
    "sphinx_immaterial",
]

html_theme = "sphinx_immaterial"

html_theme_options = {
    "repo_url": "https://github.com/avighnac/libapfs",
    "repo_name": "avighnac/libapfs",

    "icon": {
        "logo": "material/harddisk",
        "repo": "fontawesome/brands/github",
    },

    "globaltoc_collapse": True,

    "features": [
        "navigation.sections",
        "navigation.top",
        "navigation.footer",
        "toc.follow",
        "toc.sticky",
        "content.code.copy",
    ],

    "palette": [
        {
            "media": "(prefers-color-scheme: light)",
            "scheme": "default",
            "primary": "deep-purple",
            "accent": "deep-purple",
            "toggle": {
                "icon": "material/weather-night",
                "name": "Switch to dark mode",
            },
        },
        {
            "media": "(prefers-color-scheme: dark)",
            "scheme": "slate",
            "primary": "deep-purple",
            "accent": "deep-purple",
            "toggle": {
                "icon": "material/weather-sunny",
                "name": "Switch to light mode",
            },
        },
    ],
}

breathe_projects = {
    "libapfs": "../build/doxygen/xml",
}

breathe_default_project = "libapfs"

primary_domain = "cpp"
highlight_language = "cpp"

source_suffix = {
    ".rst": "restructuredtext",
    ".md": "markdown",
}

templates_path = ["_templates"]
exclude_patterns = []

html_static_path = ["_static"]

html_css_files = [
    "custom.css",
]

html_context = {
    "source_type": "github",
    "source_user": "avighnac",
    "source_repo": "libapfs",
    "source_version": "main",
    "source_docs_path": "/docs/source/",
}