#!/usr/bin/env python3
#
# TODO doctring
#
# https://gabrielelanaro.github.io/blog/2014/12/12/extract-docstrings.html
# https://docs.python.org/3.6/library/ast.html

from pathlib import Path
import ast
import re
import os
import subprocess
import itertools
import sys
from copy import deepcopy
from argparse import ArgumentParser
import yaml
from collections import defaultdict


def git_files_changed(root_path):
    """Get a list of file from git diff, based on origin/master.

    Args:
        root_path (str): directory to run in. E.g. the repo root

    Returns:
        list: paths of modified files
    """
    result = subprocess.run(
        ['git', 'diff', 'origin/master', '--name-only', '--relative'],
        stdout=subprocess.PIPE, cwd=root_path, check=True)
    return [os.path.join(root_path, path) for path in result.stdout.decode().split('\n') if path]


def all_python_files(path):
    """Get a list of all .py files recursively in a directory.

    Args:
        path (str): directory to look in

    Returns:
        list: sorted path names of .py files
    """
    return sorted(map(str, Path(path).rglob("*.py")))


def filter_type(values, type_):
    """Filter a type of values from a list.

    Args:
        values (list): list to filter
        type_ (type): type of values to keep

    Returns:
        filter: the filtered list
    """
    return filter(lambda val: isinstance(val, type_), values)


class FtestTagMap():
    """TODO"""

    def __init__(self, path=None):
        """Initialize the tag mapping.

        Args:
            path (list/str, optional): the file or directory path(s) to update from
        """
        self.__mapping = {}  # str(file_name) : str(class_name) : str(test_name) : set(tags)
        if path:
            self.update_from_path(path)

    @property
    def mapping(self):
        """Get the tag mapping.

        Returns:
            dict: mapping of str(file_name) : str(class_name) : str(test_name) : set(tags)
        """
        return deepcopy(self.__mapping)

    def update_from_path(self, path):
        """Update the mapping from a path.

        Args:
            path (list/str, optional): the file path(s) to update from

        Raises:
            ValueError: if a path is not a file
        """
        if not isinstance(path, (list, tuple)):
            path = [path]

        # Convert to realpath
        path = list(map(os.path.realpath, path))

        # Get the unique file paths
        paths = set()
        for _path in path:
            if os.path.isfile(_path):
                if _path.endswith('.py'):
                    paths.add(_path)
            else:
                raise ValueError(f'Expected file or directory: {_path}')

        # Parse each python file and update the mapping from avocado tags
        for file_path in paths:
            with open(file_path, 'r') as file:
                file_data = file.read()

            module = ast.parse(file_data)
            for class_def in filter_type(module.body, ast.ClassDef):
                for func_def in filter_type(class_def.body, ast.FunctionDef):
                    if not func_def.name.startswith('test_'):
                        continue
                    tags = self._parse_avocado_tags(ast.get_docstring(func_def))
                    self.__update(file_path, class_def.name, func_def.name, tags)

    def unique_tags(self, exclude=None):
        """Get the set of unique tags, excluding one or more paths.

        Args:
            exclude (list/str, optional): path(s) to exclude from the unique set.
                Defaults to None.

        Returns:
            set: the set of unique tags
        """
        if not exclude:
            exclude = []
        elif not isinstance(exclude, (list, tuple)):
            exclude = [exclude]
        unique_tags = set()
        realpaths = list(map(os.path.realpath, exclude))  # TODO don't use realpath
        for file_path, classes in self.__mapping.items():
            # if file_path in exclude:
            if os.path.realpath(file_path) in realpaths:
                # Exclude this one
                continue
            for functions in classes.values():
                for tags in functions.values():
                    unique_tags.update(tags)
        return unique_tags

    def minimal_tags(self, include_paths=None):
        """Get the minimal tags representing files in the mapping.

        This computes an approximate minimal - not the absolute minimal.

        Args:
            include_paths (list/str, optional): path(s) to include in the mapping.
                Defaults to None, which includes all paths

        Returns:
            list: list of sets of tags
        """
        if not include_paths:
            include_paths = []
        elif not isinstance(include_paths, (list, tuple)):
            include_paths = [include_paths]
        # Unique sets to fall back to if not tagged with the class or function name.
        # This will not be needed after all tests are tagged appropriately
        freq_stage_combos = list(map(lambda sets: set.union(*sets), itertools.product(
            [set(['pr']), set(['daily_regression']), set(['full_regression'])],
            [set(['vm']), set(['hw', 'medium']), set(['hw', 'large'])])))

        minimal_sets = []

        # TODO don't use realpath
        for idx, path in enumerate(include_paths):
            include_paths[idx] = os.path.realpath(path)

        for file_path, classes in self.__mapping.items():
            if include_paths and os.path.realpath(file_path) not in include_paths:
                continue
            # Keep track of recommended tags for each method
            file_recommended = []
            for class_name, functions in classes.items():
                for function_name, tags in functions.items():
                    # Try the class name and function name first
                    if class_name in tags:
                        file_recommended.append(set([class_name]))
                        continue
                    if function_name in tags:
                        file_recommended.append(set([function_name]))
                        continue
                    # Try using a set of tags globally unique to this test
                    globally_unique_tags = tags - self.unique_tags(exclude=file_path)
                    if globally_unique_tags and globally_unique_tags.issubset(tags):
                        file_recommended.append(globally_unique_tags)
                        continue
                    # Try one of the frequency + stage combos
                    freq_stage_tags = None
                    for _set in [*freq_stage_combos]:
                        if _set and _set.issubset(tags):
                            freq_stage_tags = _set
                            break
                    if freq_stage_tags:
                        file_recommended.append(freq_stage_tags)
                        continue
                    # Fallback to just using all of this test's tags
                    file_recommended.append(tags)

            if not file_recommended:
                continue

            # If all functions in the file have a common set of tags, use that set
            file_recommended_intersection = set.intersection(*file_recommended)
            if file_recommended_intersection:
                minimal_sets.append(file_recommended_intersection)
                continue

            # Otherwise, use tags unique to each function
            file_recommended_unique = []
            for tags in file_recommended:
                if tags not in file_recommended_unique:
                    file_recommended_unique.append(tags)
            minimal_sets.extend(file_recommended_unique)

        # Combine the minimal sets into a single set representing what avocado expects
        avocado_set = set(','.join(tags) for tags in minimal_sets)

        return avocado_set

    def __update(self, file_name, class_name, test_name, tags):
        """Update the internal mapping by appending the tags.

        Args:
            file_name (str): file name
            class_name (str): class name
            test_name (str): test name
            tags (set): set of tags to update
        """
        if not tags:
            return
        if file_name not in self.__mapping:
            self.__mapping[file_name] = {}
        if class_name not in self.__mapping[file_name]:
            self.__mapping[file_name][class_name] = {}
        if test_name not in self.__mapping[file_name][class_name]:
            self.__mapping[file_name][class_name][test_name] = set()
        self.__mapping[file_name][class_name][test_name].update(tags)

    @staticmethod
    def _parse_avocado_tags(text):
        """Parse avocado tags from a string.

        Args:
            text (str): the string to parse for tags

        Returns:
            set: the set of tags
        """
        tag_strings = re.findall(':avocado: tags=(.*)', text)
        return set(','.join(tag_strings).split(','))


def get_core_tag_mapping():
    """Map core files to tags.

    Args:
        paths (list): paths to map.

    Returns:
        dict: the mapping
    """
    with open('file_to_tag.yaml', 'r') as file:
        return yaml.safe_load(file.read())


def run_ftest_tag_lint(paths):
    all_files = []
    all_classes = defaultdict(int)
    all_methods = defaultdict(int)
    tests_wo_class_as_tag = []
    tests_wo_func_as_tag = []
    tests_wo_hw_vm_manual = []
    for file_path, classes in FtestTagMap(paths).mapping.items():
        all_files.append(file_path)
        for class_name, functions in classes.items():
            all_classes[class_name] += 1
            for method_name, tags in functions.items():
                all_methods[method_name] += 1
                if class_name not in tags:
                    tests_wo_class_as_tag.append(method_name)
                if method_name not in tags:
                    tests_wo_func_as_tag.append(method_name)
                if not set(tags).intersection(set(['vm', 'hw', 'manual'])):
                    tests_wo_hw_vm_manual.append(method_name)

    print('ftest overview')
    print(f'  {len(all_files)} test files')
    print()

    non_unique_classes = list(name for name, num in all_classes.items() if num > 1)
    print(f'  {len(non_unique_classes)} non-unique test classes: {non_unique_classes}')
    print()

    non_unique_methods = list(name for name, num in all_methods.items() if num > 1)
    print(f'  {len(non_unique_methods)} non-unique test methods: {non_unique_methods}')
    print()

    print(f'  {len(tests_wo_class_as_tag)} tests w/o class as tag: {tests_wo_class_as_tag}')
    print()
    print(f'  {len(tests_wo_func_as_tag)} tests w/o func as tag: {tests_wo_func_as_tag}')
    print()
    print(f'  {len(tests_wo_hw_vm_manual)} tests w/o hw|vm|manual: {tests_wo_hw_vm_manual}')
    print()

    # Lint fails if any of the above lists contain entries
    all_zero = not any(map(len, [
        non_unique_classes,
        non_unique_methods,
        tests_wo_class_as_tag,
        tests_wo_func_as_tag,
        tests_wo_hw_vm_manual]))
    return all_zero


def recommended_core_tags(paths):
    all_mapping = get_core_tag_mapping()
    recommended = set()
    default_tags = set(all_mapping['default'].split(' '))
    tags_per_file = all_mapping['per_path']
    for path in paths:
        # Hack - to be fixed
        if 'src/tests/ftest' in path:
            continue
        this_recommended = set()
        for _pattern, _tags in tags_per_file.items():
            if re.search(_pattern, path):
                this_recommended.update(_tags.split(' '))
        recommended |= this_recommended or default_tags
    return recommended


if __name__ == '__main__':
    parser = ArgumentParser()
    parser.add_argument(
        "--lint",
        action="store_true",
        help="run the ftest tag linter")
    parser.add_argument(
        "--paths",
        action="append",
        default=[],
        help="file paths")
    args = parser.parse_args()
    if args.lint:
        if run_ftest_tag_lint(args.paths or all_python_files('./src/tests/ftest')):
            print('ftest tag lint passed')
            sys.exit(0)
        else:
            print('ftest tag lint failed')
            sys.exit(1)

    # Get recommended tags for ftest changes
    ftest_tag_map = FtestTagMap(all_python_files('./src/tests/ftest'))
    ftest_tag_set = ftest_tag_map.minimal_tags(args.paths or git_files_changed('.'))
    print(ftest_tag_set)

    # Get recommended tags for core (non-ftest changes)
    core_tag_set = recommended_core_tags(args.paths or git_files_changed('.'))
    print(core_tag_set)
    all_tags = ' '.join(ftest_tag_set | core_tag_set)
    print('# Recommended test pragmas')
    print('# NOTE: This is still a work in progress')
    print('Test-tag: ', all_tags)


#
# ./parse_avocado_tags.py --lint
# ./parse_avocado_tags.py
# ./parse_avocado_tags.py --path /home/dbohning/daos/src/tests/ftest/erasurecode/mdtest_smoke.py
# ./parse_avocado_tags.py --path /home/dbohning/daos/src/tests/ftest/harness/skip-list.py
#
