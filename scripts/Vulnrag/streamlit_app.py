#!/usr/bin/env python3
"""
Streamlit App for Vulnrag Testing
Shows the complete process: retrieval, prompts, and LLM responses
"""

import streamlit as st
import json
import time
from pathlib import Path
import sys
import os

# Add parent directory to path
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from vulnrag_retriever import create_vulnrag_retriever
from llm_engine import create_neutral_vulnrag_engine

def setup_page():
    """Setup Streamlit page"""
    st.set_page_config(
        page_title="Vulnrag Testing App",
        page_icon="🔍",
        layout="wide"
    )
    st.title("🔍 Vulnrag Testing App")
    st.markdown("Test Vulnrag with full process visualization")

def load_components():
    """Load Vulnrag components"""
    with st.spinner("Loading Vulnrag components..."):
        retriever = create_vulnrag_retriever()
        # Enable verbose mode for detailed logging in Streamlit
        llm_engine = create_neutral_vulnrag_engine()
        return retriever, llm_engine

def analyze_code_with_details(code: str, retriever, llm_engine):
    """Analyze code and return detailed results"""
    
    # Step 1: Retrieval
    st.subheader("📊 Step 1: Retrieval Process")
    with st.spinner("Performing hybrid retrieval..."):
        start_time = time.time()
        results = retriever.hybrid_retrieval(code, top_k=5)
        retrieval_time = time.time() - start_time
        
        st.success(f"✅ Retrieval completed in {retrieval_time:.2f}s")
        
        # Display retrieval results
        col1, col2 = st.columns(2)
        
        with col1:
            st.write("**Retrieval Metadata:**")
            st.json(results.retrieval_metadata)
            
            st.write("**Best Scores:**")
            st.write(f"- VULN Score: {results.best_vuln_score:.3f}")
            st.write(f"- PATCH Score: {results.best_patch_score:.3f}")
            st.write(f"- Differential: {abs(results.best_vuln_score - results.best_patch_score):.3f}")
        
        with col2:
            st.write("**Top Results Summary:**")
            st.write(f"- Total Results: {len(results.top_results)}")
            st.write(f"- VULN Patterns: {len([r for r in results.top_results if r.pattern_type == 'VULN'])}")
            st.write(f"- PATCH Patterns: {len([r for r in results.top_results if r.pattern_type == 'PATCH'])}")
    
    # Step 2: Evidence Building
    st.subheader("🔍 Step 2: Evidence Building")
    
    evidence_for = results.evidence_for
    evidence_against = results.evidence_against
    
    col1, col2 = st.columns(2)
    
    with col1:
        st.write("**Evidence FOR Vulnerability:**")
        if evidence_for:
            for i, evidence in enumerate(evidence_for[:3], 1):
                with st.expander(f"Evidence {i} (Similarity: {evidence['similarity']:.3f})"):
                    st.write(f"**CVE:** {evidence['cve']}")
                    st.write(f"**CWE:** {evidence['cwe']}")
                    st.write(f"**Vulnerability Type:** {evidence['vulnerability_type']}")
                    st.write(f"**Trigger Condition:** {evidence['trigger_condition']}")
                    st.write(f"**Specific Behavior:** {evidence['specific_behavior']}")
                    st.write(f"**Preconditions:** {evidence['preconditions']}")
                    if evidence['code_before'] != 'N/A':
                        st.code(evidence['code_before'][:300] + "...", language="c")
        else:
            st.write("No evidence FOR vulnerability found.")
    
    with col2:
        st.write("**Evidence AGAINST Vulnerability:**")
        if evidence_against:
            for i, evidence in enumerate(evidence_against[:3], 1):
                with st.expander(f"Evidence {i} (Similarity: {evidence['similarity']:.3f})"):
                    st.write(f"**CVE:** {evidence['cve']}")
                    st.write(f"**CWE:** {evidence['cwe']}")
                    st.write(f"**Solution:** {evidence['solution']}")
                    st.write(f"**GPT Purpose:** {evidence['gpt_purpose']}")
                    if evidence['gpt_analysis'] != 'N/A':
                        st.write(f"**GPT Analysis:** {evidence['gpt_analysis'][:200]}...")
                    if evidence['code_after'] != 'N/A':
                        st.code(evidence['code_after'][:300] + "...", language="c")
        else:
            st.write("No evidence AGAINST vulnerability found.")
    
    # Step 3: LLM Analysis
    st.subheader("🤖 Step 3: LLM Analysis")
    
    with st.spinner("Performing LLM analysis..."):
        start_time = time.time()
        analysis = llm_engine.analyze_with_context(code, results)
        llm_time = time.time() - start_time
        
        st.success(f"✅ LLM analysis completed in {llm_time:.2f}s")
    
    # Display LLM results
    col1, col2, col3 = st.columns(3)
    
    with col1:
        st.write("**LLM Decision:**")
        # Debug: Show raw values
        st.write(f"**Debug - is_vulnerable:** {analysis.is_vulnerable}")
        st.write(f"**Debug - confidence_score:** {analysis.confidence_score}")
        
        if analysis.is_vulnerable:
            st.error("🚨 VULNERABLE")
        else:
            st.success("✅ SAFE")
        
        st.write(f"**Confidence:** {analysis.confidence_score:.1%}")
        st.write(f"**Risk Level:** {analysis.risk_level}")
    
    with col2:
        st.write("**Vulnerability Details:**")
        if analysis.vulnerability_type:
            st.write(f"**Type:** {analysis.vulnerability_type}")
        if analysis.cwe_prediction:
            st.write(f"**CWE:** {analysis.cwe_prediction}")
        if analysis.recommended_fix:
            st.write(f"**Fix:** {analysis.recommended_fix}")
    
    with col3:
        st.write("**Analysis Quality:**")
        st.write(f"**Evidence FOR:** {len(analysis.evidence_for)} items")
        st.write(f"**Evidence AGAINST:** {len(analysis.evidence_against)} items")
        st.write(f"**Limitations:** {len(analysis.limitations)} items")
    
    # Display full analysis
    st.write("**Full LLM Analysis:**")
    with st.expander("Complete Analysis Details"):
        st.write("**Explanation:**")
        st.write(analysis.explanation)
        
        if analysis.evidence_for:
            st.write("**Evidence FOR Vulnerability:**")
            for i, evidence in enumerate(analysis.evidence_for, 1):
                st.write(f"{i}. {evidence}")
        
        if analysis.evidence_against:
            st.write("**Evidence AGAINST Vulnerability:**")
            for i, evidence in enumerate(analysis.evidence_against, 1):
                st.write(f"{i}. {evidence}")
        
        if analysis.limitations:
            st.write("**Analysis Limitations:**")
            for i, limitation in enumerate(analysis.limitations, 1):
                st.write(f"{i}. {limitation}")
    
    return results, analysis

def main():
    """Main Streamlit app"""
    setup_page()
    
    # Load components
    retriever, llm_engine = load_components()
    st.success("✅ Vulnrag components loaded successfully!")
    
    # Input section
    st.subheader("📝 Input Code")
    
    # Sample codes
    sample_codes = {
        "Sample 1 - Safe Code": """
static void safe_function() {
    int *ptr = malloc(10);
    if (ptr != NULL) {
        // Use ptr safely
        free(ptr);
    }
}
""",
        "Sample 2 - Vulnerable Code": """
static void vulnerable_function() {
    int *ptr = malloc(10);
    // Use ptr without checking
    free(ptr);
    free(ptr);  // Double free
}
""",
        "Sample 3 - Buffer Overflow": """
static void buffer_overflow() {
    char buffer[10];
    strcpy(buffer, "This string is too long for the buffer");
}
"""
    }
    
    # Code input
    code_input = st.text_area(
        "Enter C code to analyze:",
        value=sample_codes["Sample 1 - Safe Code"],
        height=200,
        help="Enter C code to analyze for vulnerabilities"
    )
    
    # Sample selection
    selected_sample = st.selectbox(
        "Or choose a sample:",
        ["Custom Code"] + list(sample_codes.keys())
    )
    
    if selected_sample != "Custom Code":
        code_input = sample_codes[selected_sample]
        st.text_area("Selected code:", value=code_input, height=200, disabled=True)
    
    # Analysis button
    if st.button("🔍 Analyze Code", type="primary"):
        if code_input.strip():
            st.markdown("---")
            results, analysis = analyze_code_with_details(code_input, retriever, llm_engine)
            
            # Summary
            st.markdown("---")
            st.subheader("📊 Analysis Summary")
            
            col1, col2, col3 = st.columns(3)
            
            with col1:
                st.metric("Retrieval Time", f"{results.search_time_seconds:.2f}s")
            
            with col2:
                st.metric("LLM Decision", "VULNERABLE" if analysis.is_vulnerable else "SAFE")
            
            with col3:
                st.metric("Confidence", f"{analysis.confidence_score:.1%}")
            
        else:
            st.error("Please enter some code to analyze.")

if __name__ == "__main__":
    main() 