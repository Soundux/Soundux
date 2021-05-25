# Contributing

Contributions are welcome! Here's how you can help:

  - [Translations](#translations)
  - [Code](#code)
  - [Issues](#issues)
  - [Donations](#donations)

## Translations

## via Weblate (preferred)

1. Visit [our Weblate page](https://hosted.weblate.org/projects/soundux/frontend/)
2. Login / Register
3. Start translating!
    - If you are adding a translation which language or territory specific version doesn't exist yet click on **Start new translation**
    - If there already is a translation for your language and you want to correct a string just on the language and make your change

## via PR

1. [Fork the frontend](https://github.com/Soundux/soundux-ui/fork) and [clone](https://help.github.com/articles/cloning-a-repository/) your fork.
2. Start translating!
    - Add a translation file in `/src/locales/`
    - If you are adding a translation which language doesn't exist yet, name your translation `[COUNTRY_CODE].json`
    - If there already is a translation for your language and you want to add a territory specific one, name your translation `[COUNTRY_CODE]-[TERRITORY].json` so that it plays nicely with the [Implicit fallback](https://kazupon.github.io/vue-i18n/guide/fallback.html#implicit-fallback-using-locales)
    - Replace `[COUNTRY_CODE]` with your corresponding code. [See the list here](https://en.wikipedia.org/wiki/List_of_ISO_639-1_codes) (Use column `639-1`)
    - Replace `[TERRITORY]` with your corresponding territory code. [See the list here](https://en.wikipedia.org/wiki/ISO_3166-1#Officially_assigned_code_elements) (Use column `Alpha-2 code`)
    - Add the corresponding translations for your language

3. Commit your changes to a new branch (not `master`, one change per branch) and push it:
    - Use [Semantic Commit Messages](https://gist.github.com/joshbuchea/6f47e86d2510bce28f8e7f42ae84c716)

4. Once you are happy with your translation, submit a pull request.

## Code

1. [Fork](https://github.com/Soundux/Soundux/fork) the repository and [clone](https://help.github.com/articles/cloning-a-repository/) your fork.
2. Start coding!
    - Implement your feature.
    - Check your code works as expected.
    - Run the code formatter: `clang-format -i $(git ls-files "*.cpp" "*.hpp")`

3. Commit your changes to a new branch (not `master`, one change per branch) and push it:
    - Use [Semantic Commit Messages](https://gist.github.com/joshbuchea/6f47e86d2510bce28f8e7f42ae84c716)

4. Once you are happy with your changes, submit a pull request.
     - Open the pull-request.
     - Add a short description explaining briefly what you've done (or if it's a work-in-progress - what you need to do)

## Issues

1. Do a quick search in the [existing issues](https://github.com/Soundux/Soundux/issues) to check if the issue has already been reported.
2. [Open an issue](https://github.com/Soundux/Soundux/issues/new/choose) and fill in the template

After reporting you should try to answer any questions or clarifications, as this will help determine the cause of the issue.

## Donations

We are on [Ko-Fi](https://ko-fi.com/soundux)
