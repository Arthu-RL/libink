#!/usr/bin/env python3
import argparse
import subprocess as sp
import logging as log
import shutil
import os
import sys
import tempfile
import glob
import datetime
from typing import Dict, Optional, List, Tuple

# Constants
LIB = "ink"
BUILD_TYPE='release'
DEFAULT_ARCHIVE_FORMAT = "gztar"
VALID_FORMATS = ["zip", "tar", "gztar", "bztar", "xztar"]

# Configure logging
log.basicConfig(
    level=log.INFO,
    format='%(asctime)s - %(levelname)s: %(message)s',
    datefmt='%Y-%m-%d %H:%M:%S'
)

def parse_arguments() -> argparse.Namespace:
    """Parse and validate command line arguments."""
    parser = argparse.ArgumentParser(
        description="Intelligent Library Release Generator",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )
    
    parser.add_argument('-v', '--version', dest='version', type=str,
                        help='Tag version of the release (if not provided, will try to detect from git)')
    
    parser.add_argument('-b', '--build', dest='build', type=str,
                        help='Build directory', default=f"./build/{BUILD_TYPE}")
    
    parser.add_argument('-o', '--output', dest='output', type=str,
                        help='Output directory for the release archive', default='./releases')
    
    parser.add_argument('--os', dest='os', type=str,
                        help='Operational system', default='linux')
    
    parser.add_argument('--arch', dest='arch', type=str,
                        help='CPU architecture', default='amd64')
    
    parser.add_argument('-f', '--format', dest='format', type=str,
                        choices=VALID_FORMATS, default=DEFAULT_ARCHIVE_FORMAT,
                        help=f'Archive format. Options: {", ".join(VALID_FORMATS)}')
    
    parser.add_argument('-c', '--clean', dest='clean', action='store_true',
                        help='Clean build directory after successful packaging')
    
    parser.add_argument('--headers', dest='headers', type=str, 
                        help='Headers directory to include', default='./include')
    
    parser.add_argument('--debug', dest='debug', action='store_true',
                        help='Enable debug logging')
                        
    parser.add_argument('--dry-run', dest='dry_run', action='store_true',
                        help='Show what would be done without actually doing it')
    
    args = parser.parse_args()
    
    # Set logging level based on debug flag
    if args.debug:
        log.getLogger().setLevel(log.DEBUG)
        log.debug("Debug logging enabled")
    
    return args

def detect_version_from_git() -> Optional[str]:
    """Try to detect version from git tags."""
    try:
        # Get the most recent tag
        result = sp.run(
            "git describe --tags --abbrev=0", 
            shell=True, check=True, 
            capture_output=True, text=True
        )
        version = result.stdout.strip()
        if version:
            log.info(f"Detected version from git: {version}")
            return version
        else:
            log.warning("Could not detect version from git tags")
            return None
    except sp.CalledProcessError as e:
        log.warning(f"Failed to detect version from git: {e.stderr}")
        return None

def verify_build_files(build_dir: str) -> bool:
    """Verify that required build files exist."""
    lib_file = os.path.join(build_dir, 'libink.a')
    if not os.path.exists(lib_file):
        log.error(f"Library file not found: {lib_file}")
        return False
    
    log.debug(f"Found library file: {lib_file}")
    return True

def build_library(build_dir: str, dry_run: bool) -> bool:
    """Build the library using CMake."""
    log.info("Building libink...")
    
    # Check if the build directory exists, create it if it doesn't
    if not os.path.exists(build_dir):
        log.info(f"Creating build directory: {build_dir}")
        if not dry_run:
            try:
                os.makedirs(build_dir, exist_ok=True)
            except OSError as e:
                log.error(f"Failed to create build directory: {e}")
                return False
    
    # Check if CMake configuration needs to be run first (look for CMakeCache.txt)
    cmake_cache = os.path.join(build_dir, "CMakeCache.txt")
    if not os.path.exists(cmake_cache):
        log.info("CMake build directory not initialized, running configuration step...")
        
        # Run CMake configuration
        config_cmd = f"cmake -B {build_dir} -S ./ -DCMAKE_BUILD_TYPE={BUILD_TYPE.capitalize()}"
        log.info(f"Running: {config_cmd}")
        
        if dry_run:
            log.info(f"Would execute: {config_cmd}")
        else:
            try:
                result = sp.run(
                    config_cmd,
                    shell=True, check=True,
                    capture_output=True, text=True
                )
                log.debug(f"CMake configuration output:\n{result.stdout}")
            except sp.CalledProcessError as e:
                log.error(f"CMake configuration failed with error:\n{e.stderr}")
                return False
    
    # Now build the project
    build_cmd = f"cmake --build {build_dir} --target all"
    log.info(f"Running: {build_cmd}")
    
    if dry_run:
        log.info(f"Would execute: {build_cmd}")
        return True
    
    try:
        result = sp.run(
            build_cmd, 
            shell=True, check=True, 
            capture_output=True, text=True
        )
        log.debug(f"Build output:\n{result.stdout}")
        return True
    except sp.CalledProcessError as e:
        log.error(f"Build failed with error:\n{e.stderr}")
        return False

def prepare_release_package(
    version: str, 
    build_dir: str, 
    osys: str, 
    arch: str, 
    headers_dir: str,
    dry_run: bool
) -> Optional[str]:
    """
    Prepare the release package by copying necessary files.
    Returns the path to the temporary release directory.
    """
    # Create release directory name
    libdir_name = f"{LIB}-{version}_{osys}_{arch}"
    temp_dir = tempfile.mkdtemp(prefix=f"{LIB}-{BUILD_TYPE}-")
    libdir = os.path.join(temp_dir, libdir_name)
    
    log.info(f"Creating release directory: {libdir}")
    
    if dry_run:
        log.info(f"Would create directory: {libdir}")
        return libdir
    
    # Create the release directory
    os.makedirs(libdir, exist_ok=True)
    
    # Copy the library file
    lib_file = os.path.join(build_dir, 'libink.a')
    log.info(f"Copying library file: {lib_file} -> {libdir}")
    
    if not dry_run:
        try:
            shutil.copy(lib_file, libdir)
        except (shutil.Error, FileNotFoundError) as e:
            log.error(f"Failed to copy library file: {e}")
            shutil.rmtree(temp_dir)
            return None
    
    # Copy header files
    log.info(f"Copying headers: {headers_dir} -> {libdir}")
    
    if not dry_run:
        try:
            # Use copytree for directories
            headers_dest = os.path.join(libdir, os.path.basename(headers_dir))
            shutil.copytree(headers_dir, headers_dest)
        except (shutil.Error, FileNotFoundError) as e:
            log.error(f"Failed to copy headers: {e}")
            shutil.rmtree(temp_dir)
            return None
    
    # Create a README file with build information
    readme_path = os.path.join(libdir, "README.txt")
    log.info(f"Creating README: {readme_path}")
    
    if not dry_run:
        try:
            with open(readme_path, 'w') as f:
                f.write(f"# {LIB} Library {BUILD_TYPE.capitalize()}\n\n")
                f.write(f"Version: {version}\n")
                f.write(f"OS: {osys}\n")
                f.write(f"Architecture: {arch}\n")
                f.write(f"Built on: {datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n\n")
                f.write("## Contents\n\n")
                f.write("- libink.a: Static library\n")
                f.write("- include/: Header files\n")
        except IOError as e:
            log.error(f"Failed to create README: {e}")
            shutil.rmtree(temp_dir)
            return None
    
    return libdir

def create_archive(
    libdir: str, 
    output_dir: str, 
    archive_format: str,
    dry_run: bool
) -> Optional[str]:
    """
    Create an archive of the release directory.
    Returns the path to the created archive.
    """
    # Create output directory if it doesn't exist
    os.makedirs(output_dir, exist_ok=True)
    
    # Get base name for the archive (directory name)
    base_name = os.path.basename(libdir)
    output_base = os.path.join(output_dir, base_name)
    
    log.info(f"Creating {archive_format} archive: {output_base}")
    
    if dry_run:
        log.info(f"Would create archive: {output_base}.{archive_format}")
        return f"{output_base}.{archive_format}"
    
    try:
        # Create the archive from the release directory
        result = shutil.make_archive(
            base_name=output_base,
            format=archive_format,
            root_dir=os.path.dirname(libdir),
            base_dir=base_name
        )
        log.info(f"Archive created: {result}")
        return result
    except (shutil.Error, ValueError) as e:
        log.error(f"Failed to create archive: {e}")
        return None

def cleanup(temp_dir: str, build_dir: str, clean_build: bool, dry_run: bool) -> None:
    """Clean up temporary files and optionally clean build directory."""
    # Clean up temporary directory
    log.info(f"Cleaning up temporary directory: {temp_dir}")
    
    if not dry_run:
        shutil.rmtree(temp_dir)
    
    # Clean up build directory if requested
    if clean_build:
        log.info(f"Cleaning build directory: {build_dir}")
        
        if not dry_run:
            try:
                # Clean only the artifacts, not the entire build directory
                for pattern in ["*.a", "*.o", "*.so", "CMakeFiles/"]:
                    for file in glob.glob(os.path.join(build_dir, pattern)):
                        if os.path.isdir(file):
                            shutil.rmtree(file)
                        else:
                            os.remove(file)
            except (OSError, shutil.Error) as e:
                log.error(f"Failed to clean build directory: {e}")

def print_summary(
    version: str, 
    osys: str, 
    arch: str, 
    archive_path: str,
    archive_format: str
) -> None:
    """Print a summary of the release."""
    log.info("=" * 60)
    log.info(f"{LIB} Library {BUILD_TYPE.capitalize()} Summary")
    log.info("=" * 60)
    log.info(f"Version:      {version}")
    log.info(f"OS:           {osys}")
    log.info(f"Architecture: {arch}")
    log.info(f"Archive:      {os.path.basename(archive_path)}")
    log.info(f"Format:       {archive_format}")
    log.info(f"Location:     {os.path.dirname(os.path.abspath(archive_path))}")
    log.info(f"Size:         {os.path.getsize(archive_path) / 1024:.2f} KB")
    log.info("=" * 60)

def main() -> int:
    """Main function."""
    args = parse_arguments()
    
    # Get version, either from command line or from git
    version = args.version
    if not version:
        version = detect_version_from_git()
        if not version:
            log.error("Version not provided and could not be detected from git")
            return 1
    
    # Normalize paths
    build_dir = os.path.abspath(args.build)
    output_dir = os.path.abspath(args.output)
    headers_dir = os.path.abspath(args.headers)
    
    # Log configuration
    log.debug(f"Configuration:")
    log.debug(f"  Version:     {version}")
    log.debug(f"  Build dir:   {build_dir}")
    log.debug(f"  Output dir:  {output_dir}")
    log.debug(f"  OS:          {args.os}")
    log.debug(f"  Arch:        {args.arch}")
    log.debug(f"  Format:      {args.format}")
    log.debug(f"  Headers dir: {headers_dir}")
    log.debug(f"  Clean:       {args.clean}")
    log.debug(f"  Dry run:     {args.dry_run}")
    
    # Build the library
    if not build_library(build_dir, args.dry_run):
        return 1
    
    # Verify build files exist
    if not args.dry_run and not verify_build_files(build_dir):
        return 1
    
    # Prepare release package
    temp_dir = prepare_release_package(
        version, 
        build_dir, 
        args.os, 
        args.arch, 
        headers_dir,
        args.dry_run
    )
    
    if not temp_dir:
        return 1
    
    # Create archive
    archive_path = create_archive(
        temp_dir, 
        output_dir, 
        args.format,
        args.dry_run
    )
    
    if not archive_path:
        if not args.dry_run:
            shutil.rmtree(os.path.dirname(temp_dir))
        return 1
    
    # Cleanup
    cleanup(
        os.path.dirname(temp_dir), 
        build_dir, 
        args.clean,
        args.dry_run
    )
    
    # Print summary
    if not args.dry_run:
        print_summary(
            version, 
            args.os, 
            args.arch, 
            archive_path,
            args.format
        )
    
    log.info(f"{BUILD_TYPE.capitalize()} successfully created!")
    return 0

if __name__ == "__main__":
    sys.exit(main())