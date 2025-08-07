"""
Logging Configuration for KB2 Preprocessing

Centralized logging configuration for all preprocessing modules.
"""

import logging
import sys
from pathlib import Path
from typing import Optional

def setup_logging(
    level: str = "INFO",
    log_file: Optional[Path] = None,
    console_output: bool = True
) -> logging.Logger:
    """
    Setup logging configuration for preprocessing modules.
    
    Args:
        level: Logging level (DEBUG, INFO, WARNING, ERROR)
        log_file: Optional log file path
        console_output: Whether to output to console
        
    Returns:
        logging.Logger: Configured logger
    """
    # Create logger
    logger = logging.getLogger("kb2_preprocessing")
    logger.setLevel(getattr(logging, level.upper()))
    
    # Clear existing handlers
    logger.handlers.clear()
    
    # Create formatter
    formatter = logging.Formatter(
        '%(asctime)s - %(name)s - %(levelname)s - %(message)s',
        datefmt='%Y-%m-%d %H:%M:%S'
    )
    
    # Console handler
    if console_output:
        console_handler = logging.StreamHandler(sys.stdout)
        console_handler.setLevel(getattr(logging, level.upper()))
        console_handler.setFormatter(formatter)
        logger.addHandler(console_handler)
    
    # File handler
    if log_file:
        log_file.parent.mkdir(parents=True, exist_ok=True)
        file_handler = logging.FileHandler(log_file)
        file_handler.setLevel(getattr(logging, level.upper()))
        file_handler.setFormatter(formatter)
        logger.addHandler(file_handler)
    
    return logger

def get_module_logger(module_name: str) -> logging.Logger:
    """
    Get a logger for a specific module.
    
    Args:
        module_name: Name of the module
        
    Returns:
        logging.Logger: Module-specific logger
    """
    return logging.getLogger(f"kb2_preprocessing.{module_name}")

class LoggingMixin:
    """Mixin class to add logging capabilities to any class."""
    
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.logger = get_module_logger(self.__class__.__name__)
    
    def log_feature_extraction(self, features: dict, file_path: str):
        """Log feature extraction results."""
        self.logger.info(f"Extracted features from {file_path}")
        self.logger.debug(f"Feature count: {len(features)}")
        
        if 'structural_features' in features:
            structural_count = len(features['structural_features'])
            self.logger.info(f"Structural features: {structural_count}")
        
        if 'semantic_features' in features:
            semantic_count = len(features['semantic_features'])
            self.logger.info(f"Semantic features: {semantic_count}")
    
    def log_cwe_patterns(self, patterns: dict):
        """Log CWE pattern detection results."""
        if patterns:
            self.logger.info(f"Detected {len(patterns)} CWE patterns")
            for cwe_type, details in patterns.items():
                risk_score = details.get('risk_score', 0)
                self.logger.debug(f"{cwe_type}: Risk Score = {risk_score:.2f}")
        else:
            self.logger.info("No CWE patterns detected")
    
    def log_parsing_stats(self, stats: dict):
        """Log parsing statistics."""
        self.logger.info(f"Parsing completed: {stats.get('total_nodes', 0)} nodes, "
                        f"{stats.get('total_edges', 0)} edges")
    
    def log_error(self, error: Exception, context: str = ""):
        """Log error with context."""
        self.logger.error(f"Error in {context}: {str(error)}")
        self.logger.debug(f"Error details: {type(error).__name__}: {error}")

# Default logging setup
def setup_default_logging():
    """Setup default logging configuration."""
    log_dir = Path("logs")
    log_dir.mkdir(exist_ok=True)
    
    return setup_logging(
        level="INFO",
        log_file=log_dir / "kb2_preprocessing.log",
        console_output=True
    ) 