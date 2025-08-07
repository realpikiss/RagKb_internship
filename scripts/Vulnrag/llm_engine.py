"""
Vulnrag LLM Engine - Neutral and Scientific

Handles LLM integration for vulnerability analysis using VulnragRetrievalResults structure.
Uses scientifically neutral prompts to avoid confirmation bias.
"""

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
        # Use neutral analysis regardless of initial classification
        return self._analyze_neutral_context(target_code, retrieval_results)

    def _analyze_neutral_context(self, target_code: str, results) -> VulnerabilityAnalysis:
        """Perform neutral, objective analysis with all available context"""
        
        # Format evidence for context
        evidence_for_text = self._format_evidence(results.evidence_for, "FOR")
        evidence_against_text = self._format_evidence(results.evidence_against, "AGAINST")
        
        # Format differential analysis if available
        differential_analysis_text = self._format_differential_analysis(
            getattr(results, 'differential_analysis', {})
        )
        
        context = f"""
OBJECTIVE CODE SECURITY ANALYSIS

Target Code for Analysis:
```c
{target_code}
```

RETRIEVAL CONTEXT:
================================================================================
Vulnerability Pattern Score: {results.best_vuln_score:.3f}
Safe Pattern Score: {results.best_patch_score:.3f}
Score Differential: {abs(results.best_vuln_score - results.best_patch_score):.3f}

{differential_analysis_text}

CRITICAL UNDERSTANDING:
- High similarity scores often indicate common code patterns in kernel/system code
- Patches typically change only 1-2 lines from vulnerable versions  
- Similar scores for both vulnerability and patch patterns is NORMAL and expected
- Focus on identifying specific security-critical differences, not overall similarity

EVIDENCE FOR VULNERABILITY:
{evidence_for_text}

EVIDENCE AGAINST VULNERABILITY:
{evidence_against_text}

ANALYSIS REQUIREMENTS:
================================================================================
Perform an objective, evidence-based security assessment following these principles:

1. INDEPENDENT EVALUATION
   - Analyze the code independently before considering retrieval results
   - Consider that retrieval results may be incomplete or incorrect
   - Base your analysis on the specific evidence provided
   - Be especially skeptical when similarity scores are very high (>0.9) for both patterns

2. EVIDENCE-BASED REASONING
   - Identify specific code constructs that indicate vulnerability
   - Identify specific code constructs that indicate safety
   - Weigh evidence objectively without predetermined conclusions
   - Compare the target code with the provided vulnerability patterns and solutions

3. COMPREHENSIVE ASSESSMENT
   - Check for: buffer overflows, memory leaks, use-after-free, race conditions
   - Verify: input validation, bounds checking, error handling, resource management
   - Consider: edge cases, integer overflows, format string issues, injection risks
   - Use the detailed vulnerability descriptions and solutions provided

4. CRITICAL EVALUATION
   - Question whether similar patterns truly apply to this specific code
   - Consider false positive and false negative possibilities
   - Identify any limitations in the analysis
   - Compare before/after code patterns to understand the fixes
   - When patterns show high similarity to BOTH vulnerable and patched code:
     * Focus on specific security mechanisms (bounds checks, validation, locks)
     * Look for the ABSENCE of security controls rather than presence of "similar patterns"
     * Remember: most patches are minimal (1-2 line changes)

5. BALANCED REPORTING
   - Report both security strengths and weaknesses
   - Provide confidence level based on evidence quality
   - Acknowledge uncertainty where evidence is insufficient

OUTPUT REQUIREMENTS:
Provide analysis in JSON format with evidence_for, evidence_against, and limitations fields.
Base conclusions on technical evidence and the detailed patterns provided.
"""
        
        return self._call_llm_neutral(target_code, context, results.classification)

    def _format_differential_analysis(self, diff_analysis: Dict) -> str:
        """Format differential analysis for the prompt"""
        if not diff_analysis or diff_analysis.get('status') != 'analyzed':
            return "DIFFERENTIAL ANALYSIS: No analysis available"
        
        analysis_text = "DIFFERENTIAL ANALYSIS:\n"
        analysis_text += f"- Score Differential: {diff_analysis.get('score_differential', 0):.3f}\n"
        analysis_text += f"- Average Similarity: {diff_analysis.get('average_similarity', 0):.3f}\n"
        analysis_text += f"- Confidence Level: {diff_analysis.get('confidence_level', 'unknown').upper()}\n"
        
        if diff_analysis.get('same_cve_pair'):
            analysis_text += "- SAME CVE DETECTED: Comparing vulnerable code to its patch\n"
        
        if diff_analysis.get('high_similarity_both'):
            analysis_text += "- HIGH SIMILARITY WARNING: Both patterns score >0.9\n"
        
        if diff_analysis.get('security_critical_differences'):
            analysis_text += "- Security-Critical Differences Found:\n"
            for diff in diff_analysis.get('security_critical_differences', []):
                analysis_text += f"  • {diff}\n"
        
        recommendation = diff_analysis.get('recommendation', '')
        if recommendation:
            analysis_text += f"- ANALYSIS GUIDANCE: {recommendation}\n"
        
        return analysis_text

    def _format_evidence(self, evidence_list: List[Dict], evidence_type: str) -> str:
        """Format evidence list for context with rich KB2 metadata"""
        if not evidence_list:
            return f"No evidence {evidence_type.lower()} vulnerability found."
        
        # With 128k token window, we can afford more patterns
        # Use top 5 patterns for rich context (still well within limits)
        limited_evidence = evidence_list[:5]
        
        formatted = []
        for i, evidence in enumerate(limited_evidence, 1):
            if evidence_type == "FOR":
                formatted.append(f"""
Evidence {i} FOR Vulnerability:
- CVE: {evidence['cve']}
- CWE: {evidence['cwe']}
- Similarity: {evidence['similarity']:.3f}
- Vulnerability Type: {evidence.get('vulnerability_type', 'N/A')}
- Trigger Condition: {evidence.get('trigger_condition', 'N/A')}
- Specific Behavior: {evidence.get('specific_behavior', 'N/A')}
- Preconditions: {evidence.get('preconditions', 'N/A')}
- Code Before: {evidence.get('code_before', 'N/A')}
""")
            else:  # AGAINST
                formatted.append(f"""
Evidence {i} AGAINST Vulnerability:
- CVE: {evidence['cve']}
- CWE: {evidence['cwe']}
- Similarity: {evidence['similarity']:.3f}
- Solution: {evidence.get('solution', 'N/A')}
- GPT Analysis: {evidence.get('gpt_analysis', 'N/A')}
- Modified Lines: {evidence.get('modified_lines', 'N/A')}
- Code After: {evidence.get('code_after', 'N/A')}
- GPT Purpose: {evidence.get('gpt_purpose', 'N/A')}
""")
        
        return "\n".join(formatted)

    def _call_llm_neutral(self, target_code: str, context: str, classification: str) -> VulnerabilityAnalysis:
        """Call LLM with neutral prompt and enhanced response parsing"""
        
        prompt = f"""
You are an objective security researcher performing code analysis.
Your goal is to provide an unbiased, evidence-based security assessment.

{context}

RESPONSE FORMAT (JSON):
{{
    "is_vulnerable": true/false,
    "confidence_score": 0.0-1.0,
    "vulnerability_type": "specific_type" or null,
    "cwe_prediction": "CWE-XXX" or null,
    "explanation": "detailed objective analysis",
    "evidence_for": ["specific evidence supporting vulnerability"],
    "evidence_against": ["specific evidence against vulnerability"],
    "similar_patterns": ["relevant patterns from context"],
    "recommended_fix": "specific remediation" or null,
    "risk_level": "LOW"/"MEDIUM"/"HIGH"/"CRITICAL",
    "limitations": ["analysis limitations or uncertainties"]
}}

Base your analysis on:
1. Direct examination of the code structure and logic
2. Established security principles and best practices
3. Specific evidence from the code itself
4. Critical evaluation of pattern similarities

Do not assume vulnerability or safety based on retrieval scores alone.
Provide balanced assessment with clear reasoning.
"""
        
        # Log the prompt for debugging (only in verbose mode)
        if self.verbose:
            print("\n" + "="*80)
            print("🤖 LLM PROMPT:")
            print("="*80)
            estimated_tokens = len(prompt) // 4
            window_usage = (estimated_tokens / 128000) * 100
            print(f"📊 Prompt length: {len(prompt)} characters (~{estimated_tokens} tokens)")
            print(f"📊 Window usage: {window_usage:.1f}% of 128k tokens")
            print("="*80)
            print(prompt)
            print("="*80)
        
        try:
            response = self.client.chat.completions.create(
                model=self.model,
                messages=[
                    {"role": "system", "content": "You are an objective security analyst. Provide evidence-based analysis without bias."},
                    {"role": "user", "content": prompt}
                ],
                temperature=0.1,  # Low temperature for consistency
                max_tokens=1500,
                timeout=self.request_timeout
            )
            
            content = response.choices[0].message.content
            
            # Log the response for debugging (only in verbose mode)
            if self.verbose:
                print("\n" + "="*80)
                print("🤖 LLM RESPONSE:")
                print("="*80)
                print(f"Raw response length: {len(content)}")
                print(f"First 100 chars: {repr(content[:100])}")
                print("="*80)
                print(content)
                print("="*80)
            
            # Parse JSON response with validation
            try:
                if self.verbose:
                    print(f"🔍 Attempting to parse JSON...")
                    print(f"🔍 Content starts with: {repr(content[:50])}")
                
                # Clean the response - remove markdown code blocks
                cleaned_content = content.strip()
                if cleaned_content.startswith('```json'):
                    cleaned_content = cleaned_content[7:]  # Remove ```json
                if cleaned_content.endswith('```'):
                    cleaned_content = cleaned_content[:-3]  # Remove ```
                cleaned_content = cleaned_content.strip()
                
                if self.verbose:
                    print(f"🔍 Cleaned content starts with: {repr(cleaned_content[:50])}")
                
                result = json.loads(cleaned_content)
                
                # Validate and ensure all fields are present
                return VulnerabilityAnalysis(
                    is_vulnerable=result.get("is_vulnerable", False),
                    confidence_score=min(max(result.get("confidence_score", 0.5), 0.0), 1.0),
                    vulnerability_type=result.get("vulnerability_type"),
                    cwe_prediction=result.get("cwe_prediction"),
                    explanation=result.get("explanation", "Analysis completed"),
                    similar_patterns=result.get("similar_patterns", []),
                    recommended_fix=result.get("recommended_fix"),
                    risk_level=result.get("risk_level", "MEDIUM"),
                    classification=classification,
                    evidence_for=result.get("evidence_for", []),
                    evidence_against=result.get("evidence_against", []),
                    limitations=result.get("limitations", [])
                )
                
            except json.JSONDecodeError as e:
                # Structured fallback for parsing errors
                if self.verbose:
                    print(f"❌ JSON parsing failed: {str(e)}")
                    print(f"❌ Content that failed: {repr(content)}")
                return self._create_fallback_analysis(content, classification, str(e))
            
        except Exception as e:
            # Error fallback with transparency
            return VulnerabilityAnalysis(
                is_vulnerable=False,
                confidence_score=0.0,
                vulnerability_type=None,
                cwe_prediction=None,
                explanation=f"Analysis could not be completed: {str(e)}",
                similar_patterns=[],
                recommended_fix=None,
                risk_level="UNCERTAIN",
                classification=classification,
                evidence_for=[],
                evidence_against=[],
                limitations=["Complete analysis failure", str(e)]
            )

    def _create_fallback_analysis(self, content: str, classification: str, error: str) -> VulnerabilityAnalysis:
        """Create structured fallback when JSON parsing fails"""
        
        # Simple heuristic analysis as fallback
        vulnerability_keywords = ["vulnerable", "exploit", "overflow", "injection", "unsafe"]
        safety_keywords = ["safe", "validated", "checked", "secured", "protected"]
        
        vuln_count = sum(1 for keyword in vulnerability_keywords if keyword in content.lower())
        safe_count = sum(1 for keyword in safety_keywords if keyword in content.lower())
        
        is_vulnerable = vuln_count > safe_count
        confidence = 0.3  # Low confidence for fallback
        
        return VulnerabilityAnalysis(
            is_vulnerable=is_vulnerable,
            confidence_score=confidence,
            vulnerability_type="UNKNOWN",
            cwe_prediction=None,
            explanation=f"Fallback analysis performed. Original response: {content[:500]}...",
            similar_patterns=[],
            recommended_fix=None,
            risk_level="UNCERTAIN",
            classification=classification,
            evidence_for=[f"Found {vuln_count} vulnerability indicators"] if vuln_count > 0 else [],
            evidence_against=[f"Found {safe_count} safety indicators"] if safe_count > 0 else [],
            limitations=["JSON parsing failed", f"Parse error: {error}", "Using heuristic fallback"]
        )

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

def validate_analysis_consistency(analysis: VulnerabilityAnalysis) -> Dict[str, Any]:
    """
    Validate internal consistency of analysis results
    
    Args:
        analysis: VulnerabilityAnalysis to validate
        
    Returns:
        Dictionary with validation results
    """
    issues = []
    
    # Check confidence vs classification consistency
    if analysis.is_vulnerable and analysis.confidence_score < 0.3:
        issues.append("Low confidence for vulnerability determination")
    
    if not analysis.is_vulnerable and analysis.confidence_score > 0.7:
        issues.append("High confidence inconsistent with safe classification")
    
    # Check evidence balance
    evidence_ratio = len(analysis.evidence_for) / (len(analysis.evidence_against) + 1)
    if analysis.is_vulnerable and evidence_ratio < 1.0:
        issues.append("Insufficient supporting evidence for vulnerability")
    
    # Check risk level consistency
    if analysis.is_vulnerable and analysis.risk_level == "LOW":
        issues.append("Risk level may be underestimated for vulnerability")
    
    return {
        "is_consistent": len(issues) == 0,
        "issues": issues,
        "evidence_balance": evidence_ratio,
        "confidence_classification_match": (
            (analysis.is_vulnerable and analysis.confidence_score >= 0.5) or
            (not analysis.is_vulnerable and analysis.confidence_score < 0.5)
        )
    }

def quick_vulnerability_analysis(target_code: str, retrieval_results, api_key: Optional[str] = None) -> VulnerabilityAnalysis:
    """Quick end-to-end vulnerability analysis with neutral approach"""
    engine = create_neutral_vulnrag_engine(api_key=api_key)
    return engine.analyze_with_context(target_code, retrieval_results)