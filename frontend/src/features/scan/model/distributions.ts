export interface Distribution {
  id: string;
  label: string;
  versions: string[];
  accent: string;
}

export const distributions: Distribution[] = [
  { id: 'ubuntu', label: 'Ubuntu', versions: ['22.04', '24.04'], accent: '#E95420' },
  { id: 'debian', label: 'Debian', versions: ['11', '12', '13'], accent: '#D70A53' },
  { id: 'rhel', label: 'Red Hat Enterprise Linux (RHEL)', versions: ['8', '9', '10'], accent: '#EE0000' },
  { id: 'sle', label: 'SUSE Linux Enterprise (SLE)', versions: ['12', '15', '16'], accent: '#30BA78' },
  { id: 'slmicro', label: 'SUSE Linux Micro', versions: ['5', '6'], accent: '#30BA78' },
  { id: 'ol', label: 'Oracle Linux (OL)', versions: ['7', '8', '9', '10'], accent: '#F80000' },
  { id: 'amazon-linux', label: 'Amazon Linux', versions: ['2', '3', '2023'], accent: '#FF9900' },
  { id: 'anolis', label: 'Anolis OS (Alibaba Cloud)', versions: ['8', '23'], accent: '#FF6A00' },
  { id: 'kylin', label: 'Kylin Server', versions: ['6 (Sec)', '10'], accent: '#4068D4' },
  { id: 'almalinux', label: 'AlmaLinux', versions: ['9'], accent: '#18A0E8' },
  { id: 'fedora', label: 'Fedora', versions: [], accent: '#51A2DA' },
  { id: 'opensuse', label: 'openSUSE', versions: [], accent: '#73BA25' },
  { id: 'openeuler', label: 'openEuler', versions: ['2203'], accent: '#1677FF' },
  { id: 'tencentos', label: 'TencentOS', versions: ['4'], accent: '#006EFF' },
  { id: 'rhcos', label: 'Red Hat Enterprise Linux CoreOS (RHCOS)', versions: ['4'], accent: '#EE0000' },
  { id: 'openembedded', label: 'OpenEmbedded', versions: [], accent: '#5B8C5A' },
];

// Identifiants de benchmarks strictement acceptés par le backend.
// Une table explicite évite de générer des noms invalides par concaténation.
const benchmarkIds: Record<string, string> = {
  'ubuntu:22.04': 'ubuntu2204',
  'ubuntu:24.04': 'ubuntu2404',
  'debian:11': 'debian11', 'debian:12': 'debian12', 'debian:13': 'debian13',
  'rhel:8': 'rhel8', 'rhel:9': 'rhel9', 'rhel:10': 'rhel10',
  'sle:12': 'sle12', 'sle:15': 'sle15', 'sle:16': 'sle16',
  'slmicro:5': 'slmicro5', 'slmicro:6': 'slmicro6',
  'ol:7': 'ol7', 'ol:8': 'ol8', 'ol:9': 'ol9', 'ol:10': 'ol10',
  'amazon-linux:2': 'alinux2', 'amazon-linux:3': 'alinux3', 'amazon-linux:2023': 'al2023',
  'anolis:8': 'anolis8', 'anolis:23': 'anolis23',
  'kylin:6 (Sec)': 'kylinsecserver6', 'kylin:10': 'kylinserver10',
  'almalinux:9': 'almalinux9',
  'fedora:': 'fedora', 'opensuse:': 'opensuse',
  'openeuler:2203': 'openeuler2203', 'tencentos:4': 'tencentos4',
  'rhcos:4': 'rhcos4', 'openembedded:': 'openembedded',
};

export function buildBenchmarkId(distributionId: string, version: string): string {
  return benchmarkIds[`${distributionId}:${version}`] ?? '';
}