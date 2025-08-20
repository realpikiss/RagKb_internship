int kvm_sev_es_string_io(struct kvm_vcpu *vcpu, unsigned int size,
			 unsigned int port, void *data,  unsigned int count,
			 int in)
{
	vcpu->arch.sev_pio_data = data;
	vcpu->arch.sev_pio_count = count;
	return in ? kvm_sev_es_ins(vcpu, size, port)
		  : kvm_sev_es_outs(vcpu, size, port);
}
