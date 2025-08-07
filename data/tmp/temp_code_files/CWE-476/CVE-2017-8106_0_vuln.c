static int handle_invept(struct kvm_vcpu *vcpu)
{
	u32 vmx_instruction_info, types;
	unsigned long type;
	gva_t gva;
	struct x86_exception e;
	struct {
		u64 eptp, gpa;
	} operand;
	u64 eptp_mask = ((1ull << 51) - 1) & PAGE_MASK;

	if (!(nested_vmx_secondary_ctls_high & SECONDARY_EXEC_ENABLE_EPT) ||
	    !(nested_vmx_ept_caps & VMX_EPT_INVEPT_BIT)) {
		kvm_queue_exception(vcpu, UD_VECTOR);
		return 1;
	}

	if (!nested_vmx_check_permission(vcpu))
		return 1;

	if (!kvm_read_cr0_bits(vcpu, X86_CR0_PE)) {
		kvm_queue_exception(vcpu, UD_VECTOR);
		return 1;
	}

	vmx_instruction_info = vmcs_read32(VMX_INSTRUCTION_INFO);
	type = kvm_register_read(vcpu, (vmx_instruction_info >> 28) & 0xf);

	types = (nested_vmx_ept_caps >> VMX_EPT_EXTENT_SHIFT) & 6;

	if (!(types & (1UL << type))) {
		nested_vmx_failValid(vcpu,
				VMXERR_INVALID_OPERAND_TO_INVEPT_INVVPID);
		return 1;
	}

	/* According to the Intel VMX instruction reference, the memory
	 * operand is read even if it isn't needed (e.g., for type==global)
	 */
	if (get_vmx_mem_address(vcpu, vmcs_readl(EXIT_QUALIFICATION),
			vmx_instruction_info, &gva))
		return 1;
	if (kvm_read_guest_virt(&vcpu->arch.emulate_ctxt, gva, &operand,
				sizeof(operand), &e)) {
		kvm_inject_page_fault(vcpu, &e);
		return 1;
	}

	switch (type) {
	case VMX_EPT_EXTENT_CONTEXT:
		if ((operand.eptp & eptp_mask) !=
				(nested_ept_get_cr3(vcpu) & eptp_mask))
			break;
	case VMX_EPT_EXTENT_GLOBAL:
		kvm_mmu_sync_roots(vcpu);
		kvm_mmu_flush_tlb(vcpu);
		nested_vmx_succeed(vcpu);
		break;
	default:
		BUG_ON(1);
		break;
	}

	skip_emulated_instruction(vcpu);
	return 1;
}