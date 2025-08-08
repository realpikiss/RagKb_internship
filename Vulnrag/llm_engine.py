

import os
import json
import time
from typing import Dict, List, Optional, Tuple, Any
from dataclasses import dataclass
from openai import OpenAI
from pathlib import Path

@dataclass
class VulnerabilityAnalysis:
    """Structured output from LLM vulnerability analysis"""
    is_vulnerable: bool
    confidence_score: float
    vulnerability_type: Optional[str]
    cwe_prediction: Optional[str]
    explanation: str
    similar_patterns: List[str]
    recommended_fix: Optional[str]
    risk_level: str  # "LOW", "MEDIUM", "HIGH", "CRITICAL"
    classification: str  # "VULNERABLE", "SAFE", "UNCERTAIN", "NO_CONTEXT"
    evidence_for: List[str]  # Evidence supporting vulnerability
    evidence_against: List[str]  # Evidence against vulnerability
    limitations: List[str]  # Analysis limitations

class VulnragLLMEngine:
    """
    Neutral and objective LLM engine for Vulnrag system
    
    Uses neutral prompts to avoid confirmation bias.
    """
    
    def __init__(self, api_key: Optional[str] = None, model: str = "gpt-4o", verbose: bool = False):
        """
        Initialize LLM engine
        
        Args:
            api_key: OpenAI API key (or from env OPENAI_API_KEY)
            model: OpenAI model to use
            verbose: Enable detailed logging of prompts and responses
        """
        self.api_key = api_key or os.getenv('OPENAI_API_KEY')
        if not self.api_key:
            raise ValueError("OpenAI API key required. Set OPENAI_API_KEY or pass api_key parameter.")
        
        self.client = OpenAI(api_key=self.api_key)
        self.model = model
        self.max_retries = 3
        self.request_timeout = 60
        self.verbose = verbose  # Control logging verbosity

    def analyze_with_context(self, target_code: str, retrieval_results) -> VulnerabilityAnalysis:
        """
        Analyze code with retrieval context using neutral approach
        
        Args:
            target_code: Code to analyze
            retrieval_results: VulnragRetrievalResults object with evidence
            
        Returns:
            VulnerabilityAnalysis with results
        """
        # Use guided analysis instead of neutral analysis (which has issues)
        return self.analyze_with_guided_iteration(target_code, retrieval_results)
    def analyze_with_guided_iteration(self, target_code: str, retrieval_results) -> VulnerabilityAnalysis:
        """
        New guided analysis using iterative pattern matching
        
        Args:
            target_code: Code to analyze
            retrieval_results: VulnragRetrievalResults object with evidence
            
        Returns:
            VulnerabilityAnalysis with guided results
        """
        print(f"🧭 Starting guided iterative analysis...")
        print(f"📊 Available evidence: {len(retrieval_results.evidence_for)} FOR, {len(retrieval_results.evidence_against)} AGAINST")
        
        # Phase 1: Test against vulnerability patterns (evidence_for)
        vuln_result = self.analyze_iteratively(target_code, retrieval_results.evidence_for)
        
        # Phase 2: If vulnerability detected, check against patch patterns for confirmation
        if vuln_result.get("vulnerable", False):
            print(f"🔍 Vulnerability detected, verifying against patch patterns...")
            
            # Test against evidence_against (patches) to confirm fix is not present
            patch_result = self.analyze_iteratively(target_code, retrieval_results.evidence_against)
            
            # If patches don't match (fix not applied), confirm vulnerability
            if not patch_result.get("vulnerable", False):
                print(f"✅ Confirmed: Vulnerability present, fix not applied")
                
                return VulnerabilityAnalysis(
                    is_vulnerable=True,
                    confidence_score=vuln_result.get("confidence_score", 0.0),
                    vulnerability_type=vuln_result.get("detected_cwe"),
                    cwe_prediction=vuln_result.get("detected_cwe"),
                    explanation=f"Guided analysis detected: {vuln_result.get('explanation', '')}",
                    similar_patterns=[vuln_result.get("detected_cve", "")],
                    recommended_fix=f"Apply fix for {vuln_result.get('detected_cve')}",
                    risk_level="HIGH",
                    classification="VULNERABLE_GUIDED",
                    evidence_for=[f"Matched pattern: {vuln_result.get('detected_cve')}"],
                    evidence_against=[f"Fix not detected in patch analysis"],
                    limitations=["Guided analysis based on specific patterns"]
                )
            else:
                print(f"⚠️  Conflicting results: Vulnerability pattern matched but fix also detected")
                
                # Conflicting results - use confidence scores to decide
                vuln_conf = vuln_result.get("confidence_score", 0.0)
                patch_conf = patch_result.get("confidence_score", 0.0)
                
                if vuln_conf > patch_conf + 0.2:  # Vuln confidence significantly higher
                    return VulnerabilityAnalysis(
                        is_vulnerable=True,
                        confidence_score=vuln_conf * 0.8,  # Reduce confidence due to conflict
                        vulnerability_type=vuln_result.get("detected_cwe"),
                        cwe_prediction=vuln_result.get("detected_cwe"),
                        explanation=f"Conflicting patterns detected. Vulnerability pattern stronger: {vuln_result.get('explanation', '')}",
                        similar_patterns=[vuln_result.get("detected_cve", "")],
                        recommended_fix=f"Verify and apply fix for {vuln_result.get('detected_cve')}",
                        risk_level="MEDIUM",
                        classification="UNCERTAIN_CONFLICT",
                        evidence_for=[f"Matched vuln pattern: {vuln_result.get('detected_cve')}"],
                        evidence_against=[f"Also matched patch pattern: {patch_result.get('detected_cve')}"],
                        limitations=["Conflicting pattern matches detected"]
                    )
                else:
                    return VulnerabilityAnalysis(
                        is_vulnerable=False,
                        confidence_score=patch_conf,
                        vulnerability_type=None,
                        cwe_prediction=None,
                        explanation=f"Patch pattern stronger than vulnerability pattern",
                        similar_patterns=[patch_result.get("detected_cve", "")],
                        recommended_fix=None,
                        risk_level="LOW",
                        classification="SAFE_GUIDED",
                        evidence_for=[],
                        evidence_against=[f"Matched patch pattern: {patch_result.get('detected_cve')}"],
                        limitations=["Guided analysis based on specific patterns"]
                    )
        
        # Phase 3: No vulnerability detected
        print(f"✅ No vulnerability patterns matched")
        
        return VulnerabilityAnalysis(
            is_vulnerable=False,
            confidence_score=vuln_result.get("confidence_score", 0.5),
            vulnerability_type=None,
            cwe_prediction=None,
            explanation="No vulnerability patterns matched in guided analysis",
            similar_patterns=[],
            recommended_fix=None,
            risk_level="LOW",
            classification="SAFE_GUIDED",
            evidence_for=[],
            evidence_against=[f"Tested {vuln_result.get('analyzed_count', 0)} patterns"],
            limitations=["Analysis limited to available patterns"]
        )

    def analyze_with_guided_prompt(self, code: str, evidence: Dict) -> Dict:
        """
        Analyze the given code against one specific vulnerability knowledge item.
        
        Args:
            code: Code to analyze
            evidence: Single knowledge item (from evidence_for or evidence_against)
            
        Returns:
            Dict with vulnerability decision and reasoning
        """
        # Extract fields from evidence with safe fallbacks
        cve = evidence.get("cve", "N/A")
        cwe = evidence.get("cwe", "N/A")
        trigger = evidence.get("trigger_condition", "N/A")
        vuln_type = evidence.get("vulnerability_type", "N/A")
        specific_behavior = evidence.get("specific_behavior", "N/A")
        fix = evidence.get("solution", "N/A")
        preconditions = evidence.get("preconditions", "N/A")
        
        # Build focused, guided prompt
        prompt = f"""You are a vulnerability detection expert analyzing code against a specific known vulnerability pattern.

CODE TO ANALYZE:
```c
{code}
```

KNOWN VULNERABILITY PATTERN:
- CVE: {cve}
- CWE: {cwe}
- Type: {vuln_type}
- Trigger: {trigger}
- Behavior: {specific_behavior}
- Preconditions: {preconditions}

EXPECTED FIX (if vulnerable):
{fix}

ANALYSIS INSTRUCTIONS:
1. Does this code exhibit the SPECIFIC vulnerability cause described above?
2. Has the corresponding fix been applied to prevent this vulnerability?
3. Focus ONLY on this particular vulnerability pattern, not general security.

Answer in JSON format ONLY:
{{
    "vulnerable": true/false,
    "matched_cause": "yes/no/uncertain",
    "missing_fix": "yes/no/uncertain",
    "confidence_score": 0.0-1.0,
    "explanation": "brief reasoning (2-4 lines max)",
    "detected_cve": "{cve}",
    "detected_cwe": "{cwe}"
}}"""

        try:
            response = self.client.chat.completions.create(
                model=self.model,
                messages=[{"role": "user", "content": prompt}],
                temperature=0.1,
                max_tokens=500,
                timeout=30
            )
            
            content = response.choices[0].message.content.strip()
            
            # Clean potential markdown formatting
            if content.startswith('```json'):
                content = content[7:]
            if content.endswith('```'):
                content = content[:-3]
            content = content.strip()
            
            result = json.loads(content)
            
            # Validate and add metadata
            result["analyzed_evidence"] = {
                "cve": cve,
                "cwe": cwe,
                "similarity": evidence.get("similarity", 0.0)
            }
            
            return result
            
        except Exception as e:
            return {
                "vulnerable": False,
                "matched_cause": "uncertain",
                "missing_fix": "uncertain",
                "confidence_score": 0.0,
                "explanation": f"Analysis error: {str(e)}",
                "detected_cve": cve,
                "detected_cwe": cwe,
                "analyzed_evidence": {
                    "cve": cve,
                    "cwe": cwe,
                    "similarity": evidence.get("similarity", 0.0)
                },
                "error": str(e)
            }

    def analyze_iteratively(self, code: str, evidence_list: List[Dict]) -> Dict:
        """
        Perform iterative analysis against multiple vulnerability patterns.
        Stops early if a vulnerability is detected.
        
        Args:
            code: Code to analyze
            evidence_list: List of evidence items to test against
            
        Returns:
            Dict with final vulnerability decision and details
        """
        if not evidence_list:
            return {
                "vulnerable": False,
                "reason": "No vulnerability patterns provided",
                "confidence_score": 0.0,
                "analyzed_count": 0,
                "total_patterns": 0
            }
        
        analyzed_count = 0
        all_results = []
        
        print(f"🔍 Starting iterative analysis against {len(evidence_list)} patterns...")
        
        for i, evidence in enumerate(evidence_list, 1):
            cve = evidence.get("cve", "Unknown")
            similarity = evidence.get("similarity", 0.0)
            
            print(f"   📋 Testing pattern {i}/{len(evidence_list)}: {cve} (sim: {similarity:.3f})")
            
            # Analyze against this specific pattern
            result = self.analyze_with_guided_prompt(code, evidence)
            analyzed_count += 1
            all_results.append(result)
            
            # Early stop if vulnerability detected with high confidence
            if result.get("vulnerable", False) and result.get("confidence_score", 0.0) > 0.6:
                print(f"   ✅ Vulnerability detected: {cve}")
                return {
                    "vulnerable": True,
                    "reason": f"Matched vulnerability pattern: {cve}",
                    "confidence_score": result.get("confidence_score", 0.0),
                    "detected_cve": result.get("detected_cve"),
                    "detected_cwe": result.get("detected_cwe"),
                    "matched_cause": result.get("matched_cause"),
                    "missing_fix": result.get("missing_fix"),
                    "explanation": result.get("explanation"),
                    "analyzed_count": analyzed_count,
                    "total_patterns": len(evidence_list),
                    "early_stop": True,
                    "all_results": all_results
                }
        
        # No vulnerability detected after all patterns
        print(f"   ✅ Analysis complete: No vulnerability detected")
        
        # Find the highest confidence result for reporting
        best_result = max(all_results, key=lambda x: x.get("confidence_score", 0.0)) if all_results else {}
        
        return {
            "vulnerable": False,
            "reason": "No vulnerability patterns matched",
            "confidence_score": best_result.get("confidence_score", 0.0),
            "analyzed_count": analyzed_count,
            "total_patterns": len(evidence_list),
            "early_stop": False,
            "best_match": best_result.get("analyzed_evidence", {}),
            "all_results": all_results
        }

    def analyze_without_context(self, target_code: str) -> VulnerabilityAnalysis:
        """Analyze without any retrieval context - pure code analysis"""
        
        context = f"""
OBJECTIVE CODE SECURITY ANALYSIS - NO RETRIEVAL CONTEXT

Target Code for Analysis:
```c
{target_code}
```

ANALYSIS REQUIREMENTS:
================================================================================
Perform a comprehensive security analysis based solely on the code provided.

1. SYSTEMATIC EVALUATION
   - Analyze control flow and data flow
   - Identify all external inputs and their validation
   - Check memory management patterns
   - Evaluate error handling completeness

2. VULNERABILITY DETECTION
   - Buffer overflows and underflows
   - Use-after-free and double-free
   - Null pointer dereferences
   - Integer overflows and underflows
   - Format string vulnerabilities
   - Injection vulnerabilities
   - Race conditions and TOCTOU issues
   - Information disclosure risks

3. SECURITY BEST PRACTICES
   - Input validation and sanitization
   - Proper bounds checking
   - Secure memory management
   - Safe string operations
   - Proper error handling
   - Resource cleanup

4. EVIDENCE-BASED CONCLUSIONS
   - Cite specific lines or constructs
   - Explain the security impact
   - Provide confidence based on code completeness
   - Acknowledge any assumptions made

Provide objective analysis without external pattern matching.
Focus on what can be determined from the code itself.
"""
        
        return self._call_llm_neutral(target_code, context, "NO_CONTEXT")

# Enhanced utility functions

def create_neutral_vulnrag_engine(api_key: Optional[str] = None, model: str = "gpt-4o", verbose: bool = False) -> VulnragLLMEngine:
    """
    Factory function to create neutral LLM engine
    
    Args:
        api_key: OpenAI API key
        model: Model to use (gpt-4o, gpt-4, gpt-3.5-turbo, etc.)
        verbose: Enable detailed logging of prompts and responses
        
    Returns:
        Configured VulnragLLMEngine instance
    """
    return VulnragLLMEngine(api_key=api_key, model=model, verbose=verbose)