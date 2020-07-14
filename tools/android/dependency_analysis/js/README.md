# Chrome Android Dependency Analysis Visualization
## Development Setup
### Shell variables

This setup assumes Chromium is in a `cr` directory (`~/cr/src/...`). To make setup easier, you can modify and export the following variables:
```
export DEP_ANALYSIS_DIR=~/cr/src/tools/android/dependency_analysis
export DEP_ANALYSIS_BUILD_DIR=~/cr/src/out/Debug
```

### Generate JSON

See `../README.md` for instructions on using `generate_json_dependency_graph.py`, then generate a graph file in the `src` directory (`js/src/json_graph.txt`) with that exact name:

```
cd $DEP_ANALYSIS_DIR
./generate_json_dependency_graph.py --C $DEP_ANALYSIS_BUILD_DIR --o js/src/json_graph.txt
```
**The following instructions assume you are in the `dependency_analysis/js` = `$DEP_ANALYSIS_DIR/js` directory.**

### Install dependencies
You will need to install `npm` if it is not already installed (check with `npm -v`), either [from the site](https://www.npmjs.com/get-npm) or via [nvm](https://github.com/nvm-sh/nvm#about) (Node Version Manager).

To install dependencies:

```
npm install
```

### Run visualization for development

```
npm run serve
```
This command runs `webpack-dev-server` in development mode, which will bundle all the dependencies and open `localhost:8888/package_view.html`, the entry point of the bundled output. Changes made to the core JS will reload the page, and changes made to individual modules will trigger a [hot module replacement](https://webpack.js.org/concepts/hot-module-replacement/).

**To view the visualization (if it wasn't automatically opened), open `localhost:8888/package_view.html`.**

### Build the visualization
```
npm run build
```
This command runs `webpack`, which will bundle the all the dependencies into output files in the `dist/` directory. These files can then be served via other means, for example:

```
npm run serve-dist
```
This command will open a simple HTTP server serving the contents of the `dist/` directory.

To build and serve, you can execute the two commands together:
```
npm run build && npm run serve-dist
```

**To view the visualization, open `localhost:8888/package_view.html`.**

### Miscellaneous
To run [ESLint](https://eslint.org/) on the JS (and fix fixable errors) using [npx](https://www.npmjs.com/package/npx) (bundled with npm):
```
npx eslint --fix *.js
```
